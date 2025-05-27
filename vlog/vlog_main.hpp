#ifndef VLOG_MAIN_HPP 
#define VLOG_MAIN_HPP

#include <iostream>
#include <leveldb/db.h>

// os-
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libpmem2.h>

#define MPATH_PM "/home/luchao/tleveldb/pm/tmpdb"
#define MPATH_NM "/home/luchao/tleveldb/tmpdb"

#define PMDEV "/home/luchao/tleveldb/pm/"

class Kvs {
    public:
        Kvs(const std::string &path):path(path) {}
        ~Kvs() {}
    public:
        const std::string path;

        virtual bool open() = 0;
};

class mLevel : public Kvs {
    public:
        mLevel(const std::string &path):Kvs(path) {
            open();
        } 
        ~mLevel(); 
    public:
        leveldb::DB *db;
        leveldb::Options options;

        bool open();
        bool write(const std::string &key, const std::string &value);
        bool read(const std::string &key, std::string &value);
        bool _delete(const std::string &key);

        leveldb::DB *operator->() {
            return db;
        }
};

mLevel::~mLevel() {
    delete db;
}

bool mLevel::open() {
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, path, &db);
    std::cout << "db started, status: " << status.ToString() << std::endl;
    return status.ok();
}

bool mLevel::write(const std::string &key, const std::string &value) {
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, value);
    return status.ok();
}

bool mLevel::read(const std::string &key, std::string &value) {
    leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &value);
    return status.ok();
}

bool mLevel::_delete(const std::string &key) {
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    return status.ok();
}

class example {
    public:
        example() = default;
        example(std::string filepath);
        virtual ~example();

        void test();
        pmem2_granularity get_granularity(pmem2_map *map);
    
    private:
        int fd;
        pmem2_persist_fn persist_fn;
        struct pmem2_config *cfg;
        struct pmem2_source *src;
        struct pmem2_map    *map;
        struct pmem2_vm_reservation *vm_rsv;
};

pmem2_granularity example::get_granularity(pmem2_map *map) {
    return pmem2_map_get_store_granularity(map);
}

void example::test() {
    assert(get_granularity(map) == PMEM2_GRANULARITY_CACHE_LINE);

    char *addr = (char *)pmem2_map_get_address(map);
    size_t size = pmem2_map_get_size(map);
    std::cout << "map_addr = " << (uint64_t)addr << std::endl;

    uint64_t iWant = 0x44332211AABBCCDD;
    uint64_t iWant2 = 0xFABEDC22FFBBAADD;
    uint64_t *wpointer = reinterpret_cast<uint64_t *>(addr);
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
example::example(std::string filepath) {
    int err;
    filepath += "abc.txt";
    if ((fd = open(filepath.c_str(), O_RDWR)) < 0) {
        // throw std::runtime_error("fd error");
        std::cerr << "fd error";
        return ;
    }

    if (err = pmem2_config_new(&cfg)) {
        std::cerr << "cfg error";
        return ;
    }

    if (pmem2_source_from_fd(&src, fd)) {
        std::cerr << "pmem2_source_from_fd error";
        return ;
    }

    if (pmem2_config_set_required_store_granularity(
            cfg, 
            // PMEM2_GRANULARITY_BYTE
            PMEM2_GRANULARITY_CACHE_LINE
            // PMEM2_GRANULARITY_PAGE 
        )
        ) {
      std::cerr << "pmem2 granularity error";
      return;
    }

    if (err = pmem2_config_set_sharing(cfg, PMEM2_SHARED)) {
        std::cerr << "pmem2_set_sharing error" << err;
        return ;
    }

    if (err = pmem2_map_new(&map, cfg, src)) {
        std::cerr << "pmem2_map_new error = " << err;
        return ;
    }

    persist_fn = pmem2_get_persist_fn(map);
}

#endif