#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char * argv[]){
	int fwr;
	int data_srf05[1]={0};
	printf("Check mearsuring SRF05:\n");
	int fd = open("/dev/SRF05",O_RDWR);
	if(fd<0){
		printf("Cannot open file /dev/SRF05\n");
		return -1;
	}
	while(1){
		fwr = write(fd, "READ_SRF05", strlen("READ_SRF05"));
		if(fwr<0){
			printf("Cannot write to /dev/SRF05\n");
			return -1;
		}
		sleep(2);
		fwr = read(fd, data_srf05,1);
		if(fwr<0){
			printf("Cannot read to /dev/SRF05\n");
			return -1;
		}
		printf("SRF05: %d\n", data_srf05[0]);
	}
	
}