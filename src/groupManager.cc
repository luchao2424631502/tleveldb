#include "groupManager.h"

#include <libpmemobj.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <string>
#include <vector>

Group::Group(FsDaxDevice *fsdev, int gid) : pool(fsdev), _gid_dram(gid) {
    // 分区内链表不存在, 初始化链表
    if (!check_list_exist()) {
        spdlog::info("Group::Group() init_list");
        PMEMoid meta_oid = pmemobj_root(pool->pop, sizeof(listnode));
        if (OID_IS_NULL(meta_oid)) {
            spdlog::error("meta_oid is null");
            return;
        }
        listroot *meta_ptr = static_cast<listroot *>(pmemobj_direct(meta_oid));

        // PM结构赋值
        meta_ptr->gid = gid;
        meta_ptr->node_count = 1;

        PMEMoid new_node = alloc_node();  // 分配一个链表节点;
        meta_ptr->head = new_node;
        meta_ptr->tail = new_node;

        pmemobj_flush(pool->pop, meta_ptr, 64);
        pmemobj_drain(pool->pop);

        // DRAM-group 赋值
        _gid_dram = gid;
        _node_count_dram = 0;

        // DRAM-写结构 赋值
        _tail_dram = static_cast<listnode *>(pmemobj_direct(new_node));
        _offset_dram = 0;
    } else {  // 读取链表
        spdlog::info("Group::Group() read_list");
    }
}

Group::~Group() {  // 保存到 PM 中
    // 将 写指针 保存到 root 区
    delete pool;
}

listroot *Group::get_pm_root() {
    PMEMoid meta_root = pmemobj_root(pool->pop, sizeof(listroot));
    if (OID_IS_NULL(meta_root)) {
        spdlog::error("oid_is_null ");
        return nullptr;
    }
    listroot *meta_ptr = static_cast<listroot *>(pmemobj_direct(meta_root));
    return meta_ptr;
}

bool Group::check_list_exist() {
    PMEMoid meta_root = pmemobj_root(pool->pop, sizeof(listroot));
    if (OID_IS_NULL(meta_root)) {
        spdlog::error("oid_is_null ");
        return false;
    }
    listroot *meta_ptr = static_cast<listroot *>(pmemobj_direct(meta_root));
    if (meta_ptr->node_count == 0 || meta_ptr->head.off == 0 ||
            0 == meta_ptr->tail.off)
        return false;  // 链表不存在
    return true;
}

PMEMoid Group::alloc_node() {
    PMEMobjpool *pop = pool->pop;
    PMEMoid new_node_oid;
    int ret =
        pmemobj_alloc(pop, &new_node_oid, sizeof(listnode), 0,
                      listnode::listnode_construct, (void *)(&_node_count_dram));
    if (ret == -1) {
        perror("Group::alloc_node");
        throw std::runtime_error("error");
        return OID_NULL;
    }
    _node_count_dram++;
    assert(!OID_IS_NULL(new_node_oid));
    return new_node_oid;
}

void Group::free_node(PMEMoid node) {
    pmemobj_free(&node);
    _node_count_dram--;
}

void Group::add_node_to_tail(PMEMoid node_oid) {  // pass
    // 更新root区尾指针 更新当前链表尾指针
    PMEMoid meta_root = pmemobj_root(pool->pop, sizeof(listroot));
    if (OID_IS_NULL(meta_root)) {
        spdlog::error("oid_is_null ");
        return;
    }
    listroot *meta_ptr = static_cast<listroot *>(pmemobj_direct(meta_root));

    // PMEMoid node_oid = alloc_node(); 我是傻逼
    listnode *last_node = (listnode *)pmemobj_direct(meta_ptr->tail);
    last_node->next = node_oid;
    meta_ptr->tail = node_oid;
    meta_ptr->node_count++;  // 更新链表节点个数
    pmemobj_flush(pool->pop, last_node->padding, 64);
    pmemobj_flush(pool->pop, meta_ptr, 64);
    pmemobj_drain(pool->pop);
}

void Group::delete_node_from_list(PMEMoid prev, PMEMoid node) {
    PMEMoid meta_root = pmemobj_root(pool->pop, sizeof(listroot));
    if (OID_IS_NULL(meta_root)) {
        spdlog::error("oid_is_null ");
        return;
    }
    listroot *meta_ptr = static_cast<listroot *>(pmemobj_direct(meta_root));

    meta_ptr->node_count--;  // 节点个数-1

    listnode *prev_node = (listnode *)(pmemobj_direct(prev));
    listnode *cur_node = (listnode *)(pmemobj_direct(node));
    prev_node->next = cur_node->next;

    free_node(node);  // 释放当前node
    if (node.off == meta_ptr->tail.off) meta_ptr->tail = prev;

    pmemobj_flush(pool->pop, &(prev_node->next), 16);  // 刷新prev节点
    pmemobj_flush(pool->pop, meta_ptr, 64);            // 刷新元数据
    pmemobj_drain(pool->pop);
}

/* ---- GroupManager ---- */
void GroupManager::test() {
    // 分配
    // for (int i=0; i<205; i++) {
    //     partitions[0].alloc_node();
    // }
}

GroupManager::GroupManager(DevDaxDevice *devptr) : _device_ptr(devptr) {
    // 初始化
    if (1) {
        // 1. 将devdax设备划分为 n 个分区
        int partition_num = DEFAULT_PARTITION_NUMS;
        size_t partition_size = 50 * (1UL << 30);  // 50GB
        // 2. 拿到 fsdax 目录
        std::string fsdax_dir(DEFAULT_FSDAX_DIR);
        // 3. 生成 分区 文件
        for (int i = 0; i < partition_num; i++) {
            std::string filepath =
                fsdax_dir + std::string(DEFAULT_PARTITION_NAME) + std::to_string(i);
            partitions_path.push_back(filepath);
            // 创建 分区内存池 文件
            partitions.push_back(new Group(
                                     new FsDaxDevice(filepath, DEFAULT_POOL_LAYOUT_NAME, partition_size),
                                     i));
        }
    }
}

GroupManager::~GroupManager() {
    for (auto i : partitions)
        if (i) delete i;
}