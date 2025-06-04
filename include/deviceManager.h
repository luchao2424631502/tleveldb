#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <string.h>
#include <libpmemobj.h>
#include <libpmem2.h>

#include "define.h"

class Device {
public:
    Device() {}
    virtual ~Device() {};
};

/* 表示 整个devdax1.0设备 */
class DevDaxDevice : public Device {
public:
    DevDaxDevice(const std::string &devpath = DEFAULT_DAXDEV_PATH, size_t devsize = DEFAULT_DAXDEV_SIZE);
    ~DevDaxDevice();

    pmem2_granularity get_granularity(pmem2_map* map); // 拿到map的粒度
    void *get_global_address();     // 返回整个连续映射的PM空间

    void test();

private:
    void *_addr;

    int fd;
    pmem2_persist_fn persist_fn;
    struct pmem2_config* cfg;
    struct pmem2_source* src;
    struct pmem2_map* map;
    struct pmem2_vm_reservation* vm_rsv;
};

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

/* ---- libpmemobj 保存持久化元数据 ---- */
struct POOL_EMERALD_ROOT {
    size_t len;
    char buf[MAX_BUF_LEN];
};

class FsDaxDevice : public Device {
public:
    FsDaxDevice(const std::string& path, //  = DEFAULT_POOL_PATH, 必须传入
                const std::string& layout // = DEFAULT_POOL_LAYOUT_NAME
    );
    ~FsDaxDevice();

    void open(const std::string& path);
    void create(const std::string& path);
    void write();
    void read();
    void test();

private:
    PMEMobjpool* pop;
    std::string pool_path;
    std::string pool_layout_name;
};

FsDaxDevice::FsDaxDevice(const std::string& path, const std::string& layout)
    : pool_path(path), pool_layout_name(layout) {
    spdlog::info("default path = {}, layout = {}", path, layout);
}

FsDaxDevice::~FsDaxDevice() {
    if (pop) pmemobj_close(pop);
    spdlog::info("~FsDaxDevice()");
}

void FsDaxDevice::open(const std::string& path) {
    const std::string& filepath = (path.empty() ? pool_path : path);

    pop = pmemobj_open(filepath.c_str(), pool_layout_name.c_str());
    if (pop == nullptr) {
        perror("pmemobj_create: ");
        return;
    }
}

void FsDaxDevice::create(const std::string& path) {
    const std::string& filepath = (path.empty() ? pool_path : path);

    pop = pmemobj_create(filepath.c_str(), DEFAULT_POOL_LAYOUT_NAME,
                        DEFAULT_POOL_SIZE, DEFAULT_POOL_MODE);
    if (pop == nullptr) {
        perror("pmemobj_create: ");
        return;
    }
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

#endif