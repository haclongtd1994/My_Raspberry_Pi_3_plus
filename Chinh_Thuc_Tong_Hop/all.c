#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
	printf("Change to super user to run insmod\n");
	system("make all");

	int uid=1,uid_new=1;
	uid = getuid();
	printf("Current User ID: %d\n", uid);

	//changed uid to super id
	system("/usr/bin/id");
	uid_new = setuid(0);
	if(uid_new){
		printf("Cannot change UID to SU\n");
		return -1;
	}
	printf("Change to Super User: %d\n",getuid());

	system("insmod kernel_gpio_to_control.ko");
	system("insmod kernel_dht11_driver.ko");

	//change back to current uid
	system("/usr/bin/id");
	uid_new = setuid(uid);
	if(uid_new){
		printf("Cannot change back current user\n");
		return -1;
	}
	printf("Change back Current User\n");

}