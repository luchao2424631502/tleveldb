#include <spdlog/spdlog.h>
#include <gtest/gtest.h>

#include "deviceManager.h"
#include "groupManager.h"

TEST(GroupManager, size) {
    GroupManager g1;
    // g1.test();

    // spdlog::info("list_node size = {}", sizeof(listnode));
    // assert(sizeof(listnode) == (256 * (1UL << 20)));
    EXPECT_EQ(sizeof(listnode), 256*(1UL<<20)); // 链表节点大小
    EXPECT_EQ(sizeof(listroot), 56);    // 根节点大小
}

TEST(Group, insert_and_free_node) {
    GroupManager g1;

    PMEMoid node2_oid = g1[0].alloc_node();
    g1[0].add_node_to_tail(node2_oid);

    PMEMoid node3_oid = g1[0].alloc_node();
    g1[0].add_node_to_tail(node3_oid);

    PMEMoid meta_root = pmemobj_root(g1[0].get_pop(), sizeof(listroot)); 
    if (OID_IS_NULL(meta_root)) {
        spdlog::error("oid_is_null ");
        return ;
    }
    listroot *meta_ptr = static_cast<listroot*>( pmemobj_direct(meta_root) );
    PMEMoid tmp_oid = meta_ptr->head;
    EXPECT_EQ(meta_ptr->head.off, tmp_oid.off);
    EXPECT_EQ(meta_ptr->tail.off, node3_oid.off);

    PMEMoid prev_oid = tmp_oid;
    while (!OID_IS_NULL(tmp_oid)) {
        prev_oid = tmp_oid;
        listnode *node = (listnode *)( pmemobj_direct(tmp_oid) );
        tmp_oid = node->next;
    }
    EXPECT_EQ(meta_ptr->tail.off, prev_oid.off);
    EXPECT_EQ(meta_ptr->node_count, 3);

    // PASSED
    g1[0].delete_node_from_list(node2_oid, node3_oid);
    EXPECT_EQ(meta_ptr->node_count, 2);
    g1[0].delete_node_from_list(meta_ptr->head, node2_oid);
    EXPECT_EQ(meta_ptr->node_count, 1);
}

TEST(Group, read_list) {
    GroupManager g1;

    PMEMoid meta_root = pmemobj_root(g1[0].get_pop(), sizeof(listroot)); 
    if (OID_IS_NULL(meta_root)) {
        spdlog::error("oid_is_null ");
        return ;
    }
    listroot *meta_ptr = static_cast<listroot*>( pmemobj_direct(meta_root) );

    EXPECT_EQ(meta_ptr->node_count, 1);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}