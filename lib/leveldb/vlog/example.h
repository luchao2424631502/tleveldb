#ifndef VLOG_EXAMPLE_HH
#define VLOG_EXAMPLE_HH

#include <leveldb/db.h>

// os-
#include <fcntl.h>
#include <libpmem2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// third-
#include <spdlog/spdlog.h>

#define MPATH_PM "/home/luchao/tleveldb/pm/tmpdb"
#define MPATH_NM "/home/luchao/tleveldb/tmpdb"

#define PMDEV "/home/luchao/tleveldb/pm/"
#define PM1DEV "/pmem1"
#define DAXDEV "/dev/dax1.0"

class Kvs {
 public:
  Kvs(const std::string& path) : path(path) {}
  ~Kvs() {}

 public:
  const std::string path;

  virtual bool open() = 0;
};

class mLevel : public Kvs {
 public:
  mLevel(const std::string& path) : Kvs(path) { open(); }
  ~mLevel();

 public:
  leveldb::DB* db;
  leveldb::Options options;

  bool open();
  bool write(const std::string& key, const std::string& value);
  bool read(const std::string& key, std::string& value);
  bool _delete(const std::string& key);

  leveldb::DB* operator->() { return db; }
};

mLevel::~mLevel() { delete db; }

bool mLevel::open() {
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, path, &db);
  spdlog::info("db started, status: {}", status.ToString());
  return status.ok();
}

bool mLevel::write(const std::string& key, const std::string& value) {
  leveldb::Status status = db->Put(leveldb::WriteOptions(), key, value);
  return status.ok();
}

bool mLevel::read(const std::string& key, std::string& value) {
  leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &value);
  return status.ok();
}

bool mLevel::_delete(const std::string& key) {
  leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
  return status.ok();
}

class example {
 public:
  example() = default;
  example(std::string filepath);
  virtual ~example();

  void test();
  pmem2_granularity get_granularity(pmem2_map* map);

 private:
  int fd;
  pmem2_persist_fn persist_fn;
  struct pmem2_config* cfg;
  struct pmem2_source* src;
  struct pmem2_map* map;
  struct pmem2_vm_reservation* vm_rsv;
};

pmem2_granularity example::get_granularity(pmem2_map* map) {
  return pmem2_map_get_store_granularity(map);
}

void example::test() {
  assert(get_granularity(map) == PMEM2_GRANULARITY_CACHE_LINE);

  char* addr = (char*)pmem2_map_get_address(map);
  size_t size = pmem2_map_get_size(map);
  spdlog::info("map_addr = 0x{:x}", (uint64_t)addr);

  uint64_t iWant = 0x44332211AABBCCDD;
  uint64_t iWant2 = 0xFABEDC22FFBBAADD;
  uint64_t* wpointer = reinterpret_cast<uint64_t*>(addr);
  *wpointer = iWant;
  *(wpointer + 1) = iWant2;

  persist_fn(wpointer, 8);
  persist_fn(wpointer + 1, 8);
}
example::~example() {
  pmem2_map_delete(&map);
  pmem2_source_delete(&src);
  pmem2_config_delete(&cfg);
  close(fd);
}
example::example(std::string path) {
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

#ifdef DAXDEV
  size_t map_length = 500*(1UL<<30);
  if (err = pmem2_config_set_length(cfg, map_length)) {
    pmem2_perror("pmem2_config_set_length = ");
  }
#endif

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

#endif