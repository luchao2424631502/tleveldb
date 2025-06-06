#ifndef GROUPMANAGER_H
#define GROUPMANAGER_H

#include <libpmemobj.h>
#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "define.h"
#include "deviceManager.h"

/* pool的root节点 (链表头元数据) */
struct listroot {
    PMEMoid writeFront;  // 写指针 (节点+偏移)

    int node_count;  // 链表节点个数
    int gid;         // 分区号

    PMEMoid head;  // 链表头指针
    PMEMoid tail;  // 链表尾指针 (写指针 : 节点部分)
};

/* 链表节点 */
struct listnode {
    char data[LIST_NODE_SIZE_256M - CACHE_LINE_SIZE];
    char padding[CACHE_LINE_SIZE - 16];  //
    // int id;
    // int writeFront;
    PMEMoid next;

    /* 函数 -- 不占存储空间 */
    static inline int listnode_construct(PMEMobjpool *pop, void *ptr, void *arg) {
        listnode *node = (listnode *)ptr;
        node->next = OID_NULL;
        pmemobj_persist(pop, node->padding, CACHE_LINE_SIZE);
        return 0;
    }
    static inline bool canFit(
        struct listnode *a, size_t off,
        size_t size) {  // 返回1, 说明块内还有容量,可追加写入
        // return (a->writeFront + size) <= (LIST_NODE_SIZE_256M - CACHE_LINE_SIZE);
        return (off + size) <= (LIST_NODE_SIZE_256M - CACHE_LINE_SIZE);
    }
    template <class T>
    static inline bool appendWrite(PMEMobjpool *pop, struct listnode *a,
                                   size_t &off, const T *buf, size_t size) {
        if (!listnode::canFit(a, off, size))  // 空间不足
            return false;
        if (sizeof(T) > 1) {  //
            *(T *)((char *)a + off) = *buf;
            pmemobj_persist(pop, (char *)a + off, size);
        } else {  // char
            pmemobj_memcpy(pop, ((char *)a + off), buf, size, PMEMOBJ_F_MEM_TEMPORAL);
        }
        off += size;
        return true;
    }
};

/* 表示单个分区: 管理链表 */
class Group {
  public:
    Group(FsDaxDevice *fsdev, int gid);
    ~Group();

    inline PMEMobjpool *get_pop() {
        return pool->pop;
    }
    bool check_list_exist();       // 检查group分区内链表是否已经存在
    PMEMoid alloc_node();          // 分配链表节点
    void free_node(PMEMoid node);  //
    void add_node_to_tail(PMEMoid node);
    void delete_node_from_list(PMEMoid prev, PMEMoid node);

  private:
    FsDaxDevice *pool;

    int _gid;             // 组号
    int _allocated_nums;  // 已经分配出去的节点数量

    // 对group的链表追加写结构
    listnode *tail;  // DRAM写指针
    size_t offset;
};

/* ---- 对所有分区进行管理 ---- */
class GroupManager {
  public:
    GroupManager(DevDaxDevice *devptr = nullptr);
    virtual ~GroupManager();

    Group &operator[](size_t i) {
        return *(partitions[i]);
    }

  private:
    /* 暂时所有分区先位于 ext4-dax 上 */
    DevDaxDevice *_device_ptr;  //
    void *all_base_addr;

    std::vector<std::string> partitions_path;
    std::vector<Group *> partitions;

  public:
    void test();
};

#endif