#!/bin/bash

echo "Let start !"
make clean
rm test_srf05
rmmod kernel_srf05_driver.ko

make
g++ -fpermissive -w -Wall -o test_srf05 test_srf05.cpp
insmod kernel_srf05_driver.ko

./test_srf05