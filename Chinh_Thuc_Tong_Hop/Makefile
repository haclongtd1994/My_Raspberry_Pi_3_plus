obj-m+=kernel_dht11_driver.o
obj-m+=kernel_gpio_to_control.o
obj-m+=kernel_gpio_to_control_2.o
obj-m+=kernel_button_interrupts_1.o
obj-m+=kernel_button_interrupts_2.o
CFLAG:=-fpermissive -w -Wall
CLIB:=-lpthread -lcurl

all:user_update_final.cpp
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CXX) $(CFLAG) -o user_update_final user_update_final.cpp $(CLIB)
	$(CXX) $(CFLAG) -o tonghop tonghop.cpp $(CLIB)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) *.o user_update_final