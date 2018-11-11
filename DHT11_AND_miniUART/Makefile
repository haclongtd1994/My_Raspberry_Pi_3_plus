obj-m+=kernel_dht11_driver.o
CFLAG:=-fpermissive -w -Wall
CLIB:=-lpthread -lcurl

all:user_update.cpp
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CXX) $(CFLAG) -o user_update user_update.cpp $(CLIB)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) *.o user_update
