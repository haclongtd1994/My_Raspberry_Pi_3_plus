obj-m+=kernel_uart_driver.o
CFLAG:=-fpermissive -w -Wall -o
CLIB:=-lpthread

all:user_motor_driver.c
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) $(CFLAG) user_motor_driver user_motor_driver.c $(CLIB)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) *.o user_motor_driver