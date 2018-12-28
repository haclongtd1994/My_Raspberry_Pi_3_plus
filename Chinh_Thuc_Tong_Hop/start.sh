#!/bin/bash

echo $PATH
cd ~/Desktop/Chinh_Thuc_Tong_Hop/

rmmod kernel_gpio_to_control.ko
rmmod kernel_dht11_driver.ko
make clean

make all

insmod kernel_gpio_to_control.ko
insmod kernel_dht11_driver.ko


./user_update_final