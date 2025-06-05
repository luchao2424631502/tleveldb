#include "deviceManager.h"

#include <spdlog/spdlog.h>
#include <string>

/* ---- FsDax 实现 ---- */
FsDaxDevice::FsDaxDevice(const std::string& path, const std::string& layout, size_t size)
    : _pool_path(path), _pool_layout_name(layout), _size(size) {
    spdlog::info("default path = {}, layout = {}, size = {}", path, layout, _size);
    // 首先打开pool
    if (!open(path, layout)) {
        create(path, layout);
    }
}

FsDaxDevice::~FsDaxDevice() {
    if (pop) pmemobj_close(pop);
    spdlog::info("~FsDaxDevice()");
}

bool FsDaxDevice::open(const std::string& path, const std::string &layout) {
    const std::string& filepath = (path.empty() ? _pool_path : path);
    const std::string& name = (layout.empty() ? _pool_layout_name : layout);

    pop = pmemobj_open(filepath.c_str(), name.c_str());
    if (pop == nullptr) {
        perror("pmemobj_create: ");
        return false;
    }
    return true;
}

bool FsDaxDevice::create(const std::string& path, const std::string &layout) {
    const std::string& filepath = (path.empty() ? _pool_path : path);
    const std::string &name = (layout.empty() ? _pool_layout_name : layout);

    pop = pmemobj_create(filepath.c_str(), name.c_str(),
                        _size, DEFAULT_POOL_MODE);
    if (pop == nullptr) {
        perror("pmemobj_create: ");
        return false;
    }
    return true;
}

void FsDaxDevice::write() {
    PMEMoid root = pmemobj_root(pop, sizeof(POOL_EMERALD_ROOT));
    POOL_EMERALD_ROOT* rootp =
        static_cast<POOL_EMERALD_ROOT*>(pmemobj_direct(root));

    const char* buf = "emerald";
    rootp->len = strlen(buf);
    pmemobj_persist(pop, &rootp->len, sizeof(rootp->len));
    pmemobj_memcpy_persist(pop, rootp->buf, buf, strlen(buf));
}

void FsDaxDevice::read() {
    PMEMoid pm_root = pmemobj_root(pop, sizeof(POOL_EMERALD_ROOT));
    POOL_EMERALD_ROOT* root = (POOL_EMERALD_ROOT*)pmemobj_direct(pm_root);

    spdlog::warn("read(): root->buf = {}, root->len = {}", root->buf, root->len);
}

void FsDaxDevice::test() {
    int ret = -1;
    TX_BEGIN(pop) {}
    TX_ONABORT {
        spdlog::error("TX_ONABORT");
        ret = 1;
    }
    TX_ONCOMMIT {
        spdlog::info("TX_ONCOMMIT");
        ret = 0;
    }
    TX_END
    spdlog::info("TX-END ret {}", ret);
    return;
}

/* ---- DevDax 实现 ---- */
DevDaxDevice::DevDaxDevice(const std::string &path, size_t devsize) {
    int err;
    if ((fd = open(path.c_str(), O_RDWR)) < 0) {
        spdlog::info("fd error");
        return;
    }

    if (err = pmem2_config_new(&cfg)) {
        spdlog::info("cfg error");
        return;
    }

    if (pmem2_source_from_fd(&src, fd)) {
        spdlog::info("pmem2_source_from_fd error");
        return;
    }

    if (err = pmem2_config_set_length(cfg, devsize)) {
        pmem2_perror("pmem2_config_set_length = ");
        return ;
    }

    if (pmem2_config_set_required_store_granularity(cfg,
                                                    // PMEM2_GRANULARITY_BYTE
                                                    PMEM2_GRANULARITY_CACHE_LINE
                                                    // PMEM2_GRANULARITY_PAGE
                                                    )) {
      spdlog::info("pmem2 granularity error");
      return;
    }

    if (err = pmem2_config_set_sharing(cfg, PMEM2_SHARED)) {
      spdlog::info("pmem2_set_sharing error");
      return;
    }

    if (err = pmem2_map_new(&map, cfg, src)) {
      spdlog::info("pmem2_map_new error = ");
      return;
    }

    size_t alignment_size;
    pmem2_source_alignment(src, &alignment_size);
    spdlog::info("alignment_size = {}", alignment_size);

    size_t source_size, map_size;
    pmem2_source_size(src, &source_size);
    map_size = pmem2_map_get_size(map);
    spdlog::info("source_size = {}, map_size = {}", source_size, map_size);

    persist_fn = pmem2_get_persist_fn(map);
}

DevDaxDevice::~DevDaxDevice() {
    pmem2_map_delete(&map);
    pmem2_source_delete(&src);
    pmem2_config_delete(&cfg);
    close(fd);
}

pmem2_granularity DevDaxDevice::get_granularity(pmem2_map* map) {
    return pmem2_map_get_store_granularity(map);
}

void *DevDaxDevice::get_global_address() {
    _addr = pmem2_map_get_address(map);
    return _addr;
}

void DevDaxDevice::test() {
  assert(get_granularity(map) == PMEM2_GRANULARITY_CACHE_LINE);

  char* addr = (char*)pmem2_map_get_address(map);
  // size_t size = pmem2_map_get_size(map);
  // spdlog::info("map_addr = 0x{:x}", (uint64_t)addr);

  spdlog::info("var1 = 0x{:x}", *(uint64_t *)addr);
  spdlog::info("var2 = 0x{:X}", *(uint64_t *)(addr + 8));
  spdlog::info("var3 = 0x{:X}", *(uint64_t *)(addr + 16));
  uint64_t* wpointer = reinterpret_cast<uint64_t*>(addr);
  *(wpointer + 2) = 0xFFFFBBAAEECCDDAA;
  spdlog::info("var3 = 0x{:X}", *(uint64_t *)(addr + 16));
  // persist_fn(wpointer+2, 8);
  return ;

  // uint64_t iWant = 0x44332211AABBCCDD;
  // uint64_t iWant2 = 0xFABEDC22FFBBAADD;
  // uint64_t* wpointer = reinterpret_cast<uint64_t*>(addr);
  // *wpointer = iWant;
  // *(wpointer + 1) = iWant2;

  // persist_fn(wpointer, 8);
  // persist_fn(wpointer + 1, 8);
}
