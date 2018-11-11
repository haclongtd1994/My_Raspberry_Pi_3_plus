obj-m:=ebbchar.o
CFLAG:=-fpermissive -w -Wall


all:testebbchar.c
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) $(CFLAG) -o test testebbchar.c
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) *.o test