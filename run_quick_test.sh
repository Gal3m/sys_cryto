#!/bin/bash
rmmod sys_cryptocopy 2>/dev/null
make
insmod sys_cryptocopy.ko
echo hello world >in.txt
./quick_test
