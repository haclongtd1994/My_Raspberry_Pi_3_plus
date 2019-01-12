#!/bin/bash
DIR=/home/pi/Desktop/Chinh_Thuc_Tong_Hop
cd $DIR

rmmod kernel_gpio_to_control.ko
rmmod kernel_gpio_to_control_2.ko
rmmod kernel_button_interrupts_1.ko
rmmod kernel_button_interrupts_2.ko
rmmod kernel_dht11_driver.ko
make clean

make all

insmod kernel_gpio_to_control.ko
insmod kernel_gpio_to_control_2.ko
insmod kernel_button_interrupts_1.ko
insmod kernel_button_interrupts_2.ko
insmod kernel_dht11_driver.ko


$DIR/tonghop