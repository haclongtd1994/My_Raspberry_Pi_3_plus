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
	pid_t pid = fork();

	//program of process parent
	if(pid!=0){
		FILE * command;
		char cmd_kill_user;
		char line[100];
		pid_t pid_user;
		int ret,fd,n;
		char buf[256];
		int stat;
		char data[1024];
		printf("Parent with process ID: %d\n", getpid());
		//get pid of user_update_final process
		command = popen("pidof -s user_update_final","r");

		fgets(line,100,command);

		pid_user = strtoul(line,NULL,10);
		pclose(command);

		printf("pid of user_update_final: %d\n", pid_user);

		fd = open("/dev/button_2", O_RDWR);
		if(fd<0){
			perror("Failed to open device...");
			return errno;
		}
		while(1){
			ret = write(fd, "ON", 2);
			if(ret<0){
				perror("Failed to write to device...");
				return errno;
			}
			n = read(fd, (void*)buf, 255);
			if (n < 0) {
				perror("Read failed - ");
				return -1;
			}
			if(!atoi(buf)){
				system("shutdown -P now");
				return 0;
			}
			sleep(5);
		}
		//wait and check status of child process
		pid_t cpid = wait(&stat);
		if(WIFEXITED(stat)){
			if(!WEXITSTATUS(stat)){
				printf("Process child run program C: user_update_final :: process creat success and now exited\n");
				
			}
		}
	}
	else{
		

		printf("Child with process ID: %d\n", getpid());

		if(system("/home/pi/Desktop/Chinh_Thuc_Tong_Hop/user_update_final")<0) return -1;
		return 0;


	}
}