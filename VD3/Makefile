obj-m+=kernel_motor_driver.o
obj-m+=kernel_gpio_to_control.o
obj-m+=kernel_uart_driver.o
CFLAG:=-fpermissive -w -Wall
CLIB:=-lpthread -lcurl

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) $(CFLAG) -o user_motor_driver user_motor_driver.c $(CLIB)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) *.o user_motor_driver
