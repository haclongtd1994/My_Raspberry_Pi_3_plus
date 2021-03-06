#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <curl/curl.h>
#include <pthread.h>
#define RED		"\x1B[31m"
#define GRN		"\x1B[32m"
#define YEL		"\x1B[33m"
#define BLU		"\x1B[34m"
#define MAG		"\x1B[35m"
#define CYN		"\x1B[36m"
#define WHY		"\x1B[37m"
#define RESET	"\x1B[0m"

#define WEB_DEVICE	"https://luutruthietbiiot.000webhostapp.com/receive_device_id.php"

#define BUFFER	256
static char receive[BUFFER];


static int data=0;
static int data_gpio=0;
static char user_device[]="raspberry";
static char password_device[]="hunglapro13";

int access_to_server(void);
void *func_thread_motor(void *ptr);
void *func_thread_curl_motor(void *ptr);
void *func_thread_curl_gpio(void *ptr);
void *func_thread_gpio(void *ptr);

typedef struct MemoryStruct{
	char *memory;
	size_t size;
};
//Function to receive data from server
static size_t WriteMemoryCallBack(void *contents, size_t size, size_t nmemb, void* userp){
	size_t realsize = size *nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	printf("mem->size: %zu, realsize: %zu",mem->size, realsize );
	if(mem->memory == NULL){
		printf ("ERROR, Not Enough memory\r\n");
		return 0;
	}
	memcpy(&(mem->memory[mem->size]),contents,realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	return realsize;
}
int main(void){
	int access=0;
	pthread_t thread_motor;
	pthread_t thread_curl_motor;
	pthread_t thread_gpio;
	pthread_t thread_curl_gpio;

	int iret_motor;
	int iret_curl_motor;
	int iret_curl_gpio;
	int iret_gpio;

	printf("\r\n" YEL "Program user with curl and motor control" RESET "\r\n");

	curl_global_init(CURL_GLOBAL_ALL);


	iret_curl_motor = pthread_create(&thread_curl_motor, NULL, func_thread_curl_motor, NULL);
	iret_curl_gpio = pthread_create(&thread_curl_gpio, NULL, func_thread_curl_gpio, NULL);
	iret_motor = pthread_create(&thread_motor, NULL, func_thread_motor, NULL);
	iret_gpio = pthread_create(&thread_gpio, NULL, func_thread_gpio, NULL);

	printf("Ket qua luong gui du lieu: %d\r\n",thread_curl_motor);
	printf("Ket qua luong gui du lieu: %d\r\n",thread_curl_gpio);
	printf("Ket qua luong dieu khien gpio: %d\r\n",iret_gpio);
	printf("Ket qua luong motor: %d\r\n",iret_motor);

	pthread_join(thread_gpio, NULL);
	pthread_join(thread_curl_motor,NULL);
	pthread_join(thread_curl_gpio,NULL);
	pthread_join(thread_motor,NULL);

	curl_global_cleanup();
	return 0;
}

void *func_thread_curl_motor(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	int devSerialNumber = 1994;

	while(1){
		snprintf(message, 100, "tk=%s&mk=%s&serialnumber=%d&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, data);
		frame_number++;
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();

		if(curl_handle){
			curl_easy_setopt(curl_handle, CURLOPT_URL,WEB_DEVICE);
			curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, message);
			res = curl_easy_perform(curl_handle);
			if(res != CURLE_OK){
				printf(GRN"[CURL]"RESET"curl_easy_perform failed: %s\r\n",curl_easy_strerror(res));
			}else{
				printf(GRN"[CURL]"RESET"\r\nSend data OK!\r\n");
				curl_easy_cleanup(curl_handle);
			}
		}
		sleep(5);
	}
}

void *func_thread_curl_gpio(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	int devSerialNumber = 2000;

	while(1){
		snprintf(message, 100, "tk=%s&mk=%s&serialnumber=%d&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, data_gpio);
		frame_number++;
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();

		if(curl_handle){
			curl_easy_setopt(curl_handle, CURLOPT_URL, WEB_DEVICE);
			curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, message);
			res = curl_easy_perform(curl_handle);
			if(res != CURLE_OK){
				printf(GRN"[CURL]"RESET"curl_easy_perform failed: %s\r\n",curl_easy_strerror(res));
			}else{
				printf(GRN"[CURL]"RESET"\r\nSend data OK!\r\n");
				curl_easy_cleanup(curl_handle);
			}
		}
		sleep(1);
	}
}

void *func_thread_motor(void *ptr){
	int ret,fd;
	char stringtosend[BUFFER];
	printf("Starting device test code motor ...\n");
	fd = open("/dev/motor_test", O_RDWR);
	if(fd<0){
		perror("Failed to open device...");
		return errno;
	}
	while(1){
		printf("Send a message to control motor: \"ON\" or \"OFF\"");
		scanf("%[^\n]%*c", stringtosend);
		if(!strcmp(stringtosend,"ON"))	data=1;
		else if(!strcmp(stringtosend,"OFF"))	data=0;
		printf("Writing message to kernel space: [%s].\n",stringtosend);
		ret = write(fd, stringtosend, strlen(stringtosend));
		if(ret<0){
			perror("Failed to write to device...");
			return errno;
		}

		printf("Press Enter to confirm control motor %s\n",stringtosend);
		getchar();

		printf("Reading value from kernel ...\n");
		ret = read(fd, receive, BUFFER);
		if(ret<0){
			perror("Failed to read to device...");
			return errno;
		}

		printf("Value receive: %s", receive);
	}

}
void *func_thread_gpio(void *ptr){
	int ret,fd;
	printf("Starting device test code gpio ...\n");
	fd = open("/dev/gpio_test", O_RDWR);
	if(fd<0){
		perror("Failed to open device...");
		return errno;
	}
	ret = write(fd, "18", 2);
	if(ret<0){
		perror("Failed to write to device...");
		return errno;
	}
	ret = write(fd, "OUTPUT", 6);
	if(ret<0){
		perror("Failed to write to device...");
		return errno;
	}
	while(1){
		ret = write(fd, "ON", 2);
		data_gpio=1;
		if(ret<0){
			perror("Failed to write to device...");
			return errno;
		}
		sleep(5);
		ret = write(fd, "OFF", 3);
		data_gpio=0;
		if(ret<0){
			perror("Failed to write to device...");
			return errno;
		}
		sleep(5);
	}

}
