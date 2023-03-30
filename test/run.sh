#!/bin/bash

cd "$( dirname "${BASH_SOURCE[0]}" )"

export LD_LIBRARY_PATH=../bin:../jemalloc/lib

make clean;make -j

REPEAT_NUM=100

TASKS=(0 1)

for ((i=1; i<=REPEAT_NUM; ++i))
do
    for task in "${TASKS[@]}"
    do
	./init.sh
	./main $task
	./main $task
    done
done
