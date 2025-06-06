#ifndef DEFINE_H
#define DEFINE_H

#define CACHE_LINE_SIZE (64)
/* Node Configuration */
#define LIST_NODE_SIZE_64M (64 * (1UL << 20))
#define LIST_NODE_SIZE_256M (256 * (1UL << 20))
#define LIST_NODE_SIZE_1GB (1UL << 30)

/* PM Configuration */
#define MAX_BUF_LEN 1024
#define DEFAULT_POOL_PATH "/pmem0/emerald/main.obj"
#define DEFAULT_POOL_LAYOUT_NAME "emerald"
#define DEFAULT_POOL_SIZE (300 * (1UL << 30))
#define DEFAULT_POOL_MODE 0666

#define DEFAULT_DAXDEV_PATH "/dev/dax1.0"
#define DEFAULT_DAXDEV_SIZE (400 * (1UL << 30))

#define DEFAULT_FSDAX_DIR "/pmem0/emerald/"
#define DEFAULT_PARTITION_NAME "group_"

/* misc */
#define DEFAULT_PARTITION_NUMS 4

#endif