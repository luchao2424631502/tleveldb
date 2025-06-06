#include <spdlog/spdlog.h>

#include "deviceManager.h"
#include "groupManager.h"

int main() {
    spdlog::set_level(spdlog::level::err);
    GroupManager g1;
    g1.test();

    spdlog::info("list_node size = {}", sizeof(listnode));
    assert(sizeof(listnode) == (256 * (1UL << 20)));
    return 0;
}
