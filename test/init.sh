#!/bin/bash

file_name="/mnt/pmem/test"

dd if=/dev/zero of=$file_name bs=1G count=30
dd if=/dev/zero of=$file_name\_redolog bs=1G count=8
dd if=/dev/zero of=$file_name\_allocatorlog bs=1G count=8
