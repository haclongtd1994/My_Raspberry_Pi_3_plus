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
	// Running Linux OS command using system 
   system("shutdown -P now"); 
    
   return 0; 
}