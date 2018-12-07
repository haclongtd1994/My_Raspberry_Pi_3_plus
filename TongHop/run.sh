#!/bin/bash

cd ~/Desktop/DATN/My_Raspberry_Pi/TongHop/

rmmod kernel_motor_driver.ko
rmmod kernel_gpio_to_control.ko
rmmod kernel_dht11_driver.ko
make clean

make all

insmod kernel_motor_driver.ko
insmod kernel_gpio_to_control.ko
insmod kernel_dht11_driver.ko


#./main