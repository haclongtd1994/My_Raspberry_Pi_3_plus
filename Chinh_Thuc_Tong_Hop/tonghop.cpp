#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <curl/curl.h>
#include <termios.h>
#include <pthread.h>
#include <sys/wait.h>

int main(){
	char *output_file="dev/";
	pid_t pid = fork();
	//program of process parent
	if(pid!=0){
		int stat;
		char data[1024];
		printf("Parent with process ID: %d\n", getpid());
		//wait and check status of child process
		pid_t cpid = wait(&stat);
		if(WIFEXITED(stat)){
			if(!WEXITSTATUS(stat)){
				printf("Process child run start.sh success\n");
				/*FILE *fptr = fopen(output_file,"rw");
				if(fptr==NULL){
					printf("Cannot read output file\n");
					return -1;
				}
				size_t kt = fread(data,sizeof(char),1024,fptr);
				printf("%zu bytes: \n%s\n", kt, data);*/
				return 0;
			}
		}
	}
	else{
		

		printf("Child with process ID: %d\n", getpid());

		if(system("/home/pi/start.sh")<0) return -1;
		return 0;


	}
}