#include "leveldb/db.h"
#include "vlog/vlog_main.hpp"

#define PMDEV "/home/luchao/tleveldb/pm/abc.txt"

int main() {
    example e1(PMDEV);
    e1.test();
    return 0;
}
