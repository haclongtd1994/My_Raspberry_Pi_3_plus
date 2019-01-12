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

#define RED			"\x1B[31m"
#define GRN			"\x1B[32m"
#define YEL			"\x1B[33m"
#define BLU			"\x1B[34m"
#define MAG			"\x1B[35m"
#define CYN			"\x1B[36m"
#define WHY			"\x1B[37m"
#define RESET		"\x1B[0m"

#define WEB_DEVICE	"https://luutruthietbiiot.000webhostapp.com/receive_device.php"

#define BUFFER		256
static 	char 		receive[BUFFER];//using for function motor
static 	int 		Humidity=0, Temperature=0;//using for function dht11
int Humidity_Lastchange=30;
int Temperature_Lastchange=30;
int mq7_rasp=0;
int arlarm_rasp=0;
int send_dht11_T=0;
int send_dht11_H=0;
int send_mini_uart = 0;
int control_gpio =0;

int control_uart =0 ;
int send_dht11_T_stm=0;
int send_dht11_H_stm=0;
int send_mq7_stm = 0;
int send_arlarm_stm = 0;
int send_mq7_rasp = 0;
int send_arlarm_rasp = 0;

pthread_mutex_t		dht11_mutex_T 		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t		dht11_mutex_H 		= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		dht11_cond_H		= PTHREAD_COND_INITIALIZER;
pthread_cond_t		dht11_cond_T		= PTHREAD_COND_INITIALIZER;
pthread_mutex_t		mini_uart_mutex 	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		mini_uart_cond		= PTHREAD_COND_INITIALIZER;
pthread_mutex_t		gpio_mutex		 	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		gpio_cond			= PTHREAD_COND_INITIALIZER;


pthread_mutex_t		dht11_mutex_H_stm 		= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		dht11_cond_H_stm		= PTHREAD_COND_INITIALIZER;
pthread_mutex_t		dht11_mutex_T_stm 		= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		dht11_cond_T_stm		= PTHREAD_COND_INITIALIZER;
pthread_mutex_t		mq7_mutex_stm 			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t		arlarm_mutex_stm 		= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		mq7_cond_stm			= PTHREAD_COND_INITIALIZER;
pthread_cond_t		arlarm_cond_stm			= PTHREAD_COND_INITIALIZER;
pthread_mutex_t		mq7_mutex_rasp 			= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		mq7_cond_rasp			= PTHREAD_COND_INITIALIZER;
pthread_mutex_t		arlarm_mutex_rasp 			= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		arlarm_cond_rasp			= PTHREAD_COND_INITIALIZER;


static 	int 		data_gpio[2]={0,0};
static 	int 		data_gpio_last[2]={0,0};
static 	char 		user_device[]="raspberry";
static 	char 		password_device[]="hunglapro13";
static  int  		command=0;

void *func_thread_curl_gpio(void *ptr);
void *func_thread_gpio(void *ptr);
void *func_thread_curl_dht11_humidity(void *ptr);
void *func_thread_curl_dht11_temperature(void *ptr);
void *func_thread_dht11(void *ptr);
void *func_thread_mini_uart_send(void *ptr);
void *func_thread_mini_uart_receive(void *ptr);
void *func_thread_curl_stm_hum(void *ptr);
void *func_thread_curl_stm_temp(void *ptr);
void *func_thread_curl_stm_mq7(void *ptr);
void *func_thread_curl_stm_arlarm(void *ptr);
void *func_thread_monitor(void *ptr);
void *func_thread_curl_monitor(void *ptr);
void *func_thread_arlarm_rasp(void *ptr);
void *func_thread_curl_arlarm_rasp(void *ptr);

typedef struct Struct_Data_STM32{
	int hum_het,temp_het,mq7,baochay;
};
struct Struct_Data_STM32 data_stm32;

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
	pthread_t thread_gpio;
	pthread_t thread_curl_gpio;
	pthread_t thread_dht11;
	pthread_t thread_curl_dht11_humidity;
	pthread_t thread_curl_dht11_temperature;
	pthread_t thread_mini_uart_send;
	pthread_t thread_mini_uart_receive;
	pthread_t thread_curl_stm_hum;
	pthread_t thread_curl_stm_temp;
	pthread_t thread_curl_stm_mq7;
	pthread_t thread_curl_stm_arlarm;
	pthread_t thread_monitor;
	pthread_t thread_curl_monitor;
	pthread_t thread_arlarm_rasp;
	pthread_t thread_curl_arlarm_rasp;

	int iret_curl_gpio;
	int iret_gpio;
	int iret_curl_dht11_humidity;
	int iret_curl_dht11_temperature;
	int iret_dht11;
	int iret_mini_uart_send;
	int iret_mini_uart_receive;
	int iret_curl_stm_hum;
	int iret_curl_stm_temp;
	int iret_curl_stm_mq7;
	int iret_curl_stm_arlarm;
	int iret_monitor;
	int iret_curl_monitor;
	int iret_arlarm_rasp;
	int iret_curl_arlarm_rasp;

	printf("\r\n" YEL "Program for FINAL THESIS of HUNG" RESET "\r\n");

	curl_global_init(CURL_GLOBAL_ALL);

	iret_curl_gpio = pthread_create(&thread_curl_gpio, NULL, func_thread_curl_gpio, NULL);
	iret_gpio = pthread_create(&thread_gpio, NULL, func_thread_gpio, NULL);
	iret_dht11 = pthread_create(&thread_dht11, NULL, func_thread_dht11, NULL);
	iret_curl_dht11_humidity = pthread_create(&thread_curl_dht11_humidity, NULL, func_thread_curl_dht11_humidity, NULL);
	iret_curl_dht11_temperature = pthread_create(&thread_curl_dht11_temperature, NULL, func_thread_curl_dht11_temperature, NULL);
	iret_mini_uart_send = pthread_create(&thread_mini_uart_send, NULL, func_thread_mini_uart_send, NULL);
	iret_mini_uart_receive = pthread_create(&thread_mini_uart_receive, NULL, func_thread_mini_uart_receive, NULL);
	iret_curl_stm_hum = pthread_create(&thread_curl_stm_hum, NULL, func_thread_curl_stm_hum, NULL);
	iret_curl_stm_temp = pthread_create(&thread_curl_stm_temp, NULL, func_thread_curl_stm_temp, NULL);
	iret_curl_stm_mq7 = pthread_create(&thread_curl_stm_mq7, NULL, func_thread_curl_stm_mq7, NULL);
	iret_curl_stm_arlarm = pthread_create(&thread_curl_stm_arlarm, NULL, func_thread_curl_stm_arlarm, NULL);
	iret_monitor = pthread_create(&thread_monitor, NULL, func_thread_monitor, NULL);
	iret_curl_monitor = pthread_create(&thread_curl_monitor, NULL, func_thread_curl_monitor, NULL);
	iret_arlarm_rasp = pthread_create(&thread_arlarm_rasp, NULL, func_thread_arlarm_rasp, NULL);
	iret_curl_arlarm_rasp = pthread_create(&thread_curl_arlarm_rasp, NULL, func_thread_curl_arlarm_rasp, NULL);

	printf("Ket qua luong gui du lieu: %d\r\n",iret_curl_gpio);
	printf("Ket qua luong dieu khien gpio: %d\r\n",iret_gpio);
	printf("Ket qua luong gui du lieu dht11 humidity: %d\r\n", iret_curl_dht11_humidity);
	printf("Ket qua luong gui du lieu dht11 temperature: %d\r\n", iret_curl_dht11_temperature);
	printf("Ket qau luong doc du lieu dht11: %d\r\n", iret_dht11);
	printf("Ket qau luong doc du lieu mini uart send: %d\r\n", iret_mini_uart_send);
	printf("Ket qau luong doc du lieu mini uart receive: %d\r\n", iret_mini_uart_receive);
	printf("Ket qau luong doc du lieu curl Temperature of STM: %d\r\n", iret_curl_stm_hum);
	printf("Ket qau luong doc du lieu curl Humidity of STM: %d\r\n", iret_curl_stm_temp);
	printf("Ket qau luong doc du lieu curl MQ7 of STM: %d\r\n", iret_curl_stm_mq7);
	printf("Ket qau luong doc du lieu curl FloorArlarm of STM: %d\r\n", iret_curl_stm_arlarm);
	printf("Ket qau luong doc du lieu MQ7 of Rasp: %d\r\n", iret_monitor);
	printf("Ket qau luong doc du lieu curl MQ7 of Rasp: %d\r\n", iret_curl_monitor);
	printf("Ket qau luong doc du lieu FloorArlarm of Rasp: %d\r\n", iret_arlarm_rasp);
	printf("Ket qau luong doc du lieu curl FloorArlarm of Rasp: %d\r\n", iret_curl_arlarm_rasp);

	pthread_join(thread_gpio, NULL);
	pthread_join(thread_curl_gpio,NULL);
	pthread_join(thread_dht11, NULL);
	pthread_join(thread_curl_dht11_humidity, NULL);
	pthread_join(thread_curl_dht11_temperature, NULL);
	pthread_join(thread_mini_uart_send, NULL);
	pthread_join(thread_mini_uart_receive, NULL);
	pthread_join(thread_curl_stm_hum, NULL);
	pthread_join(thread_curl_stm_temp, NULL);
	pthread_join(thread_curl_stm_mq7, NULL);
	pthread_join(thread_curl_stm_arlarm, NULL);
	pthread_join(thread_monitor, NULL);
	pthread_join(thread_curl_monitor, NULL);
	pthread_join(thread_arlarm_rasp, NULL);
	pthread_join(thread_curl_arlarm_rasp, NULL);

	curl_global_cleanup();
	return 0;
}

void *func_thread_arlarm_rasp(void *ptr){
	int ret,fd;
	printf("Starting device test code gpio ...\n");
	fd = open("/dev/gpio_test_2", O_RDWR);
	if(fd<0){
		perror("Failed to open device...");
		return errno;
	}
	ret = write(fd, "17", 2);
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
		if((mq7_rasp==1)&&(Temperature_Lastchange>50)){
			arlarm_rasp =1;
			ret = write(fd, "ON", 2);
			if(ret<0){
				perror("Failed to write to device...");
				return errno;
			}
		}
		else{
			arlarm_rasp = 0;
			ret = write(fd, "OFF", 2);
			if(ret<0){
				perror("Failed to write to device...");
				return errno;
			}
		}
		pthread_mutex_lock(&arlarm_mutex_rasp);
		send_arlarm_rasp=1;
		pthread_cond_signal(&arlarm_cond_rasp);
		pthread_mutex_unlock(&arlarm_mutex_rasp);
		sleep(6);
	}
}
void *func_thread_curl_arlarm_rasp(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "FloorArlarm";

	while(1){
		pthread_mutex_lock(&arlarm_mutex_rasp);//giu khoa mutex
		while(send_arlarm_rasp==0) pthread_cond_wait(&arlarm_cond_rasp,&arlarm_mutex_rasp);	//cho dieu kien
		pthread_mutex_unlock(&arlarm_mutex_rasp);//mo khoa mutex
		send_arlarm_rasp=0;
		snprintf(message, 100, "tang2=0&tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, arlarm_rasp);
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();
		frame_number++;
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
	}
}
void *func_thread_monitor(void *ptr){
	int ret,fd,n;
	char buf[256];
	printf("Starting device test button ...\n");
	fd = open("/dev/button_1", O_RDWR);
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
		if(!atoi(buf))	mq7_rasp=1;
		else			mq7_rasp=0;
		pthread_mutex_lock(&mq7_mutex_rasp);
		send_mq7_rasp=1;
		pthread_cond_signal(&mq7_cond_rasp);
		pthread_mutex_unlock(&mq7_mutex_rasp);
		sleep(5);
	}
}
void *func_thread_curl_monitor(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "MQ7";

	while(1){
		pthread_mutex_lock(&mq7_mutex_rasp);//giu khoa mutex
		while(send_mq7_rasp==0) pthread_cond_wait(&mq7_cond_rasp,&mq7_mutex_rasp);	//cho dieu kien
		pthread_mutex_unlock(&mq7_mutex_rasp);//mo khoa mutex
		send_mq7_rasp=0;
		snprintf(message, 100, "tang2=0&tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, mq7_rasp);
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();
		frame_number++;
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
	}
}

void *func_thread_curl_stm_hum(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "Humidity";

	while(1){
		pthread_mutex_lock(&dht11_mutex_H_stm);//giu khoa mutex
		while(send_dht11_H_stm==0) pthread_cond_wait(&dht11_cond_H_stm,&dht11_mutex_H_stm);	//cho dieu kien
		pthread_mutex_unlock(&dht11_mutex_H_stm);//mo khoa mutex
		send_dht11_H_stm=0;
		snprintf(message, 100, "tang2=1&tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, data_stm32.hum_het);
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();
		frame_number++;
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
			pthread_mutex_lock(&dht11_mutex_T_stm);
			send_dht11_T_stm=1;
			pthread_cond_signal(&dht11_cond_T_stm);
			pthread_mutex_unlock(&dht11_mutex_T_stm);
		}
	}
}
void *func_thread_curl_stm_temp(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "Temperature";

	while(1){
		pthread_mutex_lock(&dht11_mutex_T_stm);//giu khoa mutex
		while(send_dht11_T_stm==0) pthread_cond_wait(&dht11_cond_T_stm,&dht11_mutex_T_stm);	//cho dieu kien
		pthread_mutex_unlock(&dht11_mutex_T_stm);//mo khoa mutex
		send_dht11_T_stm=0;
		snprintf(message, 100, "tang2=1&tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, data_stm32.temp_het);
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();
		frame_number++;
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
			pthread_mutex_lock(&mq7_mutex_stm);
			send_mq7_stm=1;
			pthread_cond_signal(&mq7_cond_stm);
			pthread_mutex_unlock(&mq7_mutex_stm);
		}
	}
}
void *func_thread_curl_stm_mq7(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "MQ7";

	while(1){
		pthread_mutex_lock(&mq7_mutex_stm);//giu khoa mutex
		while(send_mq7_stm==0) pthread_cond_wait(&mq7_cond_stm,&mq7_mutex_stm);	//cho dieu kien
		pthread_mutex_unlock(&mq7_mutex_stm);//mo khoa mutex
		send_mq7_stm=0;
		snprintf(message, 100, "tang2=1&tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, data_stm32.mq7);
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();
		frame_number++;
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
			pthread_mutex_lock(&arlarm_mutex_stm);
			send_arlarm_stm=1;
			pthread_cond_signal(&arlarm_cond_stm);
			pthread_mutex_unlock(&arlarm_mutex_stm);
		}
	}
}
void *func_thread_curl_stm_arlarm(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "FloorArlarm";

	while(1){
		pthread_mutex_lock(&arlarm_mutex_stm);//giu khoa mutex
		while(send_arlarm_stm==0) pthread_cond_wait(&arlarm_cond_stm,&arlarm_mutex_stm);	//cho dieu kien
		pthread_mutex_unlock(&arlarm_mutex_stm);//mo khoa mutex
		send_arlarm_stm=0;
		snprintf(message, 100, "tang2=1&tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, data_stm32.baochay);
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();
		frame_number++;
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
	}
}
void *func_thread_mini_uart_receive(void *ptr){
	int fd;
	char buf[256];
	char *token;
	int n,i=0;
	Struct_Data_STM32 temp;
	// Open the Port. We want read/write, no "controlling tty" status, and open it no matter what state DCD is in
	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		perror("open_port: Unable to open /dev/ttyS0 - ");
		return(-1);
	}
	fcntl(fd, F_SETFL, 0);
	while(true){
		pthread_mutex_lock(&mini_uart_mutex);//giu khoa mutex
		while(send_mini_uart==0) pthread_cond_wait(&mini_uart_cond,&mini_uart_mutex);	//cho dieu kien
		pthread_mutex_unlock(&mini_uart_mutex);//mo khoa mutex
  		
  		printf("DA DEN HAM READ\n");
		n = read(fd, (void*)buf, 255);
		if (n < 0) {
			perror("Read failed - ");
			return -1;
		}
		else if (n == 0)
			printf("No data on port\n");
		else {
			printf("Ham Receive: %s\n", buf);
			if(command==0){
				//process data receive from ST protocol: "dht11Hum dht11Hum dht11Temp dht11Temp MQ7 ArlarmFire"
				token = strtok(buf," ");
				while((token!=NULL)){
					switch(i){
						case 0:			temp.hum_het = atoi(token);
										break;
						case 1:			temp.temp_het = atoi(token);
										break;
						case 2:			temp.mq7 = atoi(token);
										break;
						case 3:			temp.baochay = atoi(token);
										break;

					}
					i++;
					token = strtok(NULL," ");
				}
				if(i==4){
					data_stm32.hum_het = temp.hum_het; data_stm32.temp_het = temp.temp_het; 
					data_stm32.mq7 = temp.mq7; data_stm32.baochay = temp.baochay; 
					printf("STM32 send: %d, %d, %d, %d\n", data_stm32.hum_het,data_stm32.temp_het,
															 data_stm32.mq7, data_stm32.baochay);
				}
				else{
					printf("STM send DATA not good\n");
				}
				i=0;
			}
			else{
				if(!strcmp(buf,"OK\n")){
					printf("Command Send OK\n");
				}
			}
		}
		send_mini_uart = 0;
		pthread_mutex_lock(&dht11_mutex_H_stm);
		send_dht11_H_stm=1;
		pthread_cond_signal(&dht11_cond_H_stm);
		pthread_mutex_unlock(&dht11_mutex_H_stm);

	}
}


void *func_thread_mini_uart_send(void *ptr){
	int test=0;
	int fd;
	char buf[256];
  	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
  	if (fd == -1) {
   	 	perror("open_port: Unable to open /dev/ttyS0 - ");
    	return(-1);
  	}
  	fcntl(fd, F_SETFL, 0);
  	sleep(1);
  	int n = write(fd,"SEND\r\n", strlen("SEND\r\n"));
  	if (n < 0) {
    	perror("Write failed - ");
    	return -1;
  	}

  	n = read(fd, (void*)buf, 255);
	if (n < 0) {
		perror("Read failed - ");
		return -1;
	}
	else if (n == 0)
		printf("No data on port\n");
	else {
		printf("%i bytes read : %s %d\r\n", n, buf, (int *)buf);
		if(!strcmp(buf,"OK\n")){
			while(true){
		  		memset(buf,0,strlen(buf));
		  		if(!command){
		  			strcpy(buf,"DATA\r\n");}
		  		else	{snprintf(buf,256,"COMMAND %d\r\n",data_gpio_last[1]);command=0;}
		  		n = write(fd,buf, strlen(buf));
		  		if (n < 0) {
			    	perror("Write failed - ");
			    	return -1;
			  	}
			  	pthread_mutex_lock(&mini_uart_mutex);
				send_mini_uart=1;
				pthread_cond_signal(&mini_uart_cond);
				pthread_mutex_unlock(&mini_uart_mutex);
			  	
				while(send_mini_uart);
				sleep(5);
			}
		}
		else{
			printf("Data from STM32 not ok\n");
			return -2;
		}
	}
  	
} 

void *func_thread_curl_gpio(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	int i=0;
	char *token;
	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "GPIO";

	while(1){
		MemoryStruct chunk;
		chunk.memory = malloc(1);
		chunk.size = 0;
		snprintf(message, 100, "tk=%s&mk=%s&request=1",user_device, password_device);
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();

		if(curl_handle){
			curl_easy_setopt(curl_handle, CURLOPT_URL, "https://luutruthietbiiot.000webhostapp.com/receive_device_status.php");
			curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
			curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,WriteMemoryCallBack);
			curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,(void *)&chunk);
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, message);
			res = curl_easy_perform(curl_handle);
			if(res != CURLE_OK){
				printf(GRN"[CURL]"RESET"curl_easy_perform failed: %s\r\n",curl_easy_strerror(res));
			}else{
				printf(GRN"[CURL]"RESET"\r\nSend data OK!\r\n");
				printf(GRN "[CURL] " RESET "Phan hoi tu server: [%d size] %s\r\n",chunk.size,chunk.memory);
				token = strtok(chunk.memory," ");
				while((token!=NULL)){
					data_gpio[i] = atoi(token);
					i++;
					token = strtok(NULL," ");
				}
				i=0;
				if(data_gpio[0]!=data_gpio_last[0]){
					data_gpio_last[0] = data_gpio[0];
					pthread_mutex_lock(&gpio_mutex);
					control_gpio=1;
					pthread_cond_signal(&gpio_cond);
					pthread_mutex_unlock(&gpio_mutex);
				}
				if(data_gpio[1]!=data_gpio_last[1]){
					data_gpio_last[1] = data_gpio[1];
					command=1;
				}
				curl_easy_cleanup(curl_handle);
			}
		}
		sleep(1);
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
		pthread_mutex_lock(&gpio_mutex);//giu khoa mutex
		while(control_gpio==0) pthread_cond_wait(&gpio_cond,&gpio_mutex);	//cho dieu kien
		pthread_mutex_unlock(&gpio_mutex);//mo khoa mutex
		control_gpio=0;
		if(data_gpio[0]){
			ret = write(fd, "ON", 2);
			if(ret<0){
				perror("Failed to write to device...");
				return errno;
			}
		}
		else{
			ret = write(fd, "OFF", 3);
			if(ret<0){
				perror("Failed to write to device...");
				return errno;
			}
		}
	}
}
void *func_thread_dht11(void *ptr){
	int ret,fd;
	int dht11_data[4]={0};

	printf("Starting device dht11 test ... \n");
	fd = open("/dev/DHT11", O_RDWR);
	if(fd<0){
		printf("Failed to open device DHT11\n");
		return errno;
	}
	while(1){
		
		ret = write(fd, "READ_DHT11", 10);
		if(ret<0){
			perror("Failed to write data to kernel\n");
			return errno;
		}
		sleep(5);
		ret = read(fd, dht11_data, 4);
		if(ret < 0){
			printf("Failed to read from kernel\n");
			return errno;
		}
		printf("%d  %d  %d  %d\r\n",dht11_data[0],dht11_data[1],dht11_data[2],dht11_data[3]);
		if(dht11_data[1]>=5)	dht11_data[0]++;
		if(dht11_data[3]>=5)	dht11_data[2]++;
		Humidity = dht11_data[0];
		Temperature = dht11_data[2];
		printf("Humidity: %d, Temperature: %d\r\n", Humidity, Temperature);
		
		if(Temperature!=Temperature_Lastchange || Humidity != Humidity_Lastchange){
			Temperature_Lastchange = Temperature;
			Humidity_Lastchange = Humidity;
			
		}
		pthread_mutex_lock(&dht11_mutex_H);
		send_dht11_H=1;
		pthread_cond_signal(&dht11_cond_H);
		pthread_mutex_unlock(&dht11_mutex_H);
		pthread_mutex_lock(&dht11_mutex_T);
		send_dht11_T=1;
		pthread_cond_signal(&dht11_cond_T);
		pthread_mutex_unlock(&dht11_mutex_T);
	}
}

void *func_thread_curl_dht11_humidity(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "Humidity";

	while(1){
		pthread_mutex_lock(&dht11_mutex_H);//giu khoa mutex
		while(send_dht11_H==0) pthread_cond_wait(&dht11_cond_H,&dht11_mutex_H);	//cho dieu kien
		pthread_mutex_unlock(&dht11_mutex_H);//mo khoa mutex
		snprintf(message, 100, "tang2=0&tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, Humidity);
		frame_number++;
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();
		send_dht11_H=0;

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
	}
}
void *func_thread_curl_dht11_temperature(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "Temperature";

	while(1){
		pthread_mutex_lock(&dht11_mutex_T);
		while(send_dht11_T==0) pthread_cond_wait(&dht11_cond_T,&dht11_mutex_T);
		pthread_mutex_unlock(&dht11_mutex_T);
		snprintf(message, 100, "tang2=0&tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, Temperature);
		frame_number++;
		printf(GRN"[CURL]"RESET"message:%s\r\n",message);
		curl_handle = curl_easy_init();
		send_dht11_T =0;

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
	}
}