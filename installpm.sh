#!/bin/bash

unmount_pm() {
    if [ ! -z "$(mount | egrep 'pmem')" ]; then
        echo "存在"
        path="$(mount | egrep 'pmem' | awk '{print $3}')"
        sudo umount $path
    fi
}

unmount_pm

# sudo mkfs.ext4 -O ^has_journal,^metadata_csum /dev/pmem0

dev0="pmem0"
dev1="pmem1"
home=$(pwd)
mount_dir="${home}/pm"


if [ ! -d "$home/pm" ]
then
    mkdir pm
fi

is_mount=$(mount | grep -w "${dev0}")
if [ ! "$is_mount" ]
then
    sudo mount -o dax,noatime,nodiratime,norelatime /dev/${dev0} ${mount_dir}
    sudo chown -R $USER:$USER ${mount_dir}
fi