#!/bin/bash

echo $PATH
cd ~/Desktop/DATN/My_Raspberry_Pi/TongHop/

rmmod kernel_gpio_to_control.ko
rmmod kernel_dht11_driver.ko
make clean

make all

insmod kernel_gpio_to_control.ko
insmod kernel_dht11_driver.ko


./main