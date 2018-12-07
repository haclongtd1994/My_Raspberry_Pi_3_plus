obj-m+=kernel_gpio_to_control.o
obj-m+=kernel_dht11_driver.o
CFLAG:=-fpermissive -w -Wall
CLIB:=-lpthread -lcurl

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CXX) $(CFLAG) -o main main.cpp $(CLIB)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) *.o kernel_gpio_to_control.o kernel_dht11_driver.o
	$(RM)	main
