#!/bin/bash

# 是否格式化文件系统: sudo mkfs.ext4 -O ^has_journal,^metadata_csum /dev/pmem0

dev=${1:-"pmem0"}
mount_dir="/${dev}"

unmount_pm() {
    devname="$1"
    if [ ! -z "$(mount | egrep $devname)" ]; then
        echo "存在"
        path="$(mount | egrep $devname | awk '{print $3}')"
        sudo umount $path
    fi
}

unmount_pm $dev

if [ ! -d "$mount_dir" ]
then
    mkdir $mount_dir
fi

is_mount=$(mount | grep -w "${dev}")
if [ ! "$is_mount" ]
then
    sudo mount -o dax,noatime,nodiratime,norelatime /dev/${dev} ${mount_dir}
    sudo chown -R $USER:$USER ${mount_dir}
fi