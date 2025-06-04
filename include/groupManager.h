#ifndef GROUPMANAGER_H
#define GROUPMANAGER_H

#include "define.h"
#include "deviceManager.h"

class GroupManager {
public:
    GroupManager(DevDaxDevice *devptr);
    virtual ~GroupManager();

private:
    DevDaxDevice *_device_ptr; // 所有分区位于dev PM
    void *all_base_addr;

};

GroupManager::GroupManager(DevDaxDevice *devptr):_device_ptr(devptr) {
    // 1. 将devdax设备划分为 n 个分区

    int partition_num = 4;
    
}

GroupManager::~GroupManager() {

}

#endif