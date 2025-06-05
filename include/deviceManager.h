#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <string.h>
#include <libpmemobj.h>
#include <libpmem2.h>

#include "define.h"
#include "metadataManager.h"
#include <string>

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


/* ---- libpmemobj 保存持久化元数据 ---- */
class FsDaxDevice : public Device {
public:
    FsDaxDevice(const std::string& path, //  = DEFAULT_POOL_PATH, 必须传入
                const std::string& layout, // = DEFAULT_POOL_LAYOUT_NAME
                size_t size
    );
    ~FsDaxDevice();

    bool open(const std::string& path, const std::string &layout);
    bool create(const std::string& path, const std::string &layout);
    void write();
    void read();
    void test();

    PMEMobjpool* pop;
private:
    std::string _pool_path;
    std::string _pool_layout_name;
    size_t _size;
};

#endif