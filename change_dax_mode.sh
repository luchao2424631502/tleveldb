#!/bin/bash

# ./change_dax_mode.sh namespace1.0 mode

namespace=${1:-"namespace1.0"}
dev=$(ndctl list -n $namespace | egrep -o "pmem.")

modify_to_devdax() {
    namespace=$1
    if [ -z $namespace ]; then
        return 
    fi
    sudo ndctl create-namespace -fe $namespace -m devdax -a 4K -M mem
    sudo chown -R $USER:$USER /dev/dax1.0
    echo "success to DEVDAX"
}

modify_to_fsdax() {
    namespace=$1
    if [ -z $namespace ]; then
        return 
    fi
    sudo ndctl create-namespace -fe $namespace -m fsdax -a 4K -M mem
    echo "success to FSDAX"
}

mode=${2:-"fsdax"}
if [ "$mode" == "devdax" ]; then
    modify_to_fsdax
else
    modify_to_devdax
fi