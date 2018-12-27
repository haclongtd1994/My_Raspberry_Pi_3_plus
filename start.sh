#!/bin/bash
PATH:=/home/phanthanhhung/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/home/phanthanhhung/scripts
export PATH=$PATH:~/scripts

echo $PATH
cd ~/Desktop/DATN/My_Raspberry_Pi/TongHop/

rmmod kernel_gpio_to_control.ko
rmmod kernel_dht11_driver.ko
make clean

make all

insmod kernel_gpio_to_control.ko
insmod kernel_dht11_driver.ko


./main