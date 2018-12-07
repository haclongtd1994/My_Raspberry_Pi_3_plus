#!/bin/bash

cd ~/Desktop/DATN/My_Raspberry_Pi/TongHop/

rmmod kernel_gpio_to_control.ko
rmmod kernel_dht11_driver.ko
make clean