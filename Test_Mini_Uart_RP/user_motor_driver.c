#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#define RED		"\x1B[31m"
#define GRN		"\x1B[32m"
#define YEL		"\x1B[33m"
#define BLU		"\x1B[34m"
#define MAG		"\x1B[35m"
#define CYN		"\x1B[36m"
#define WHY		"\x1B[37m"
#define RESET	"\x1B[0m"




void *func_thread_uart(void *ptr);

int main(void){
	pthread_t thread_uart;

	int iret_uart;

	printf("\r\n" YEL "Program user with uart" RESET "\r\n");


	iret_uart = pthread_create(&thread_gpio, NULL, func_thread_uart, NULL);

	printf("Ket qua luong dieu khien gpio: %d\r\n",iret_uart);

	pthread_join(thread_uart, NULL);

	return 0;
}

void *func_thread_uart(void *ptr){
	int ret,fd;
	printf("Starting device test code gpio ...\n");
	fd = open("/dev/uart_test_kernel", O_RDWR);
	if(fd<0){
		perror("Failed to open device...");
		return errno;
	}
	while(1){
		ret = write(fd, "SEND", 4);
		if(ret<0){
			perror("Failed to write to device...");
			return errno;
		}
		sleep(5);
	}

}
