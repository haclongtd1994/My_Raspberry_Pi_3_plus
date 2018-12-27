#!/bin/bash
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/home/phanthanhhung
cd ~/Desktop/DATN/My_Raspberry_Pi/TongHop/

rmmod kernel_gpio_to_control.ko
rmmod kernel_dht11_driver.ko
make clean