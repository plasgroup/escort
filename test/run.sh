#!/bin/bash

nvm_file_path=${1:-"/mnt/pmem/test"}

cd "$( dirname "${BASH_SOURCE[0]}" )"

export LD_LIBRARY_PATH=../bin:../jemalloc/lib

make clean;make -j

REPEAT_NUM=100

TASKS=(0 1)

for ((i=1; i<=REPEAT_NUM; ++i))
do
    for task in "${TASKS[@]}"
    do
	if [ -f $nvm_file_path ]; then
	    dd if=/dev/zero of=$nvm_file_path bs=1k count=1 conv=notrunc
	else
	    ../tools/mknvmfile $nvm_file_path
	fi
	./main $nvm_file_path $task
	./main $nvm_file_path $task
    done
done
