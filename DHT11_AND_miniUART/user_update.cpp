#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>			//lock process
#include <termios.h>		//system call
#include <ctype.h>
#include <curl/curl.h>		//GET, POST to my Web API
#include <pthread.h>		//muti thread

#define RED			"\x1B[31m"
#define GRN			"\x1B[32m"
#define YEL			"\x1B[33m"
#define BLU			"\x1B[34m"
#define MAG			"\x1B[35m"
#define CYN			"\x1B[36m"
#define WHY			"\x1B[37m"
#define RESET		"\x1B[0m"

#define WEB_DEVICE	"https://luutruthietbiiot.000webhostapp.com/receive_device_id.php"
//variable for DHT11
static 	int 		Humidity=0, Temperature=0;//using for function dht11
int Humidity_Lastchange=0;
int Temperature_Lastchange=0;

pthread_mutex_t		dht11_mutex_hum 	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t		dht11_mutex_temp 	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		dht11_cond			= PTHREAD_COND_INITIALIZER;
pthread_mutex_t     uart_mutex			= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t    	uart_cond			= PTHREAD_COND_INITIALIZER;


static int send_hum=0,send_temp=0;
//Variable to connect to mysql
static 	char 		user_device[]="raspberry";
static 	char 		password_device[]="hunglapro13";
//Function pthread
void *func_thread_curl_send_data_only(void *ptr);
void *func_thread_dht11(void *ptr);
void *func_thread_uart_receive(void *ptr);
void *func_thread_uart_transmit(void *ptr);
//Variable for uart
static char message_uart_send[64]="Hello Computer\n";
static char message_uart_receive[64]="\0";
static int send_check=0;


int main(void){
	pthread_t thread_dht11;
	pthread_t thread_curl_dht11_humidity;
	pthread_t thread_curl_dht11_temperature;
	pthread_t thread_uart_receive;
	pthread_t thread_uart_transmit;

	int iret_curl_dht11_humidity;
	int iret_curl_dht11_temperature;
	int iret_dht11;
	int iret_uart_receive;
	int iret_uart_transmit;

	printf("\r\n" YEL "Program user with curl and motor control" RESET "\r\n");

	curl_global_init(CURL_GLOBAL_ALL);

	iret_dht11 = pthread_create(&thread_dht11, NULL, func_thread_dht11, NULL);
	iret_curl_dht11_humidity = pthread_create(&thread_curl_dht11_humidity, NULL, func_thread_curl_send_data_only,(void *)"Humidity");
	iret_curl_dht11_temperature = pthread_create(&thread_curl_dht11_temperature, NULL, func_thread_curl_send_data_only,(void *)"Temperature");
	iret_uart_receive = pthread_create(&thread_uart_receive, NULL, func_thread_uart_receive, NULL);
	iret_uart_transmit = pthread_create(&thread_uart_transmit, NULL, func_thread_uart_transmit, NULL);

	printf("Ket qua luong gui du lieu dht11 humidity: %d\r\n", iret_curl_dht11_humidity);
	printf("Ket qua luong gui du lieu dht11 temperature %d\r\n", iret_curl_dht11_temperature);
	printf("Ket qua luong doc du lieu dht11: %d\r\n", iret_dht11);
	printf("Ket qua luong gui du lieu uart: %d\r\n", iret_uart_transmit);
	printf("Ket qua luong nhan du lieu uart: %d\r\n", iret_uart_receive);

	pthread_join(thread_dht11, NULL);
	pthread_join(thread_curl_dht11_humidity, NULL);
	pthread_join(thread_curl_dht11_temperature, NULL);
	pthread_join(thread_uart_transmit, NULL);
	pthread_join(thread_uart_receive, NULL);

	curl_global_cleanup();
	return 0;
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
		pthread_mutex_lock(&dht11_mutex_hum);
		pthread_mutex_lock(&dht11_mutex_temp);
		ret = write(fd, "READ_DHT11", 10);
		if(ret<0){
			perror("Failed to write data to kernel\n");
			return errno;
		}
		sleep(1);
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
		printf("Check Humidity: %d, Temperature: %d\r\n", Humidity_Lastchange, Temperature_Lastchange);
		
		if(Temperature!=Temperature_Lastchange || Humidity != Humidity_Lastchange){
			printf("Call condition variable\n");
			Temperature_Lastchange = Temperature;
			Humidity_Lastchange = Humidity;
			pthread_cond_broadcast(&dht11_cond);
		}
		pthread_mutex_unlock(&dht11_mutex_hum);
		pthread_mutex_unlock(&dht11_mutex_temp);
	}
}

void *func_thread_curl_send_data_only(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char *devSerialNumber = (char *)ptr;

	while(1){
		if(!strcmp(ptr,"Humidity")){
			pthread_mutex_lock(&dht11_mutex_hum);//giu khoa mutex
			while(Temperature==Temperature_Lastchange || Humidity == Humidity_Lastchange) pthread_cond_wait(&dht11_cond,&dht11_mutex_hum);	//cho dieu kien
			snprintf(message, 100, "tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, Humidity);
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
			pthread_mutex_unlock(&dht11_mutex_hum);//mo khoa mutex
		}
		else if(!strcmp(ptr, "Temperature")){
			pthread_mutex_lock(&dht11_mutex_temp);//giu khoa mutex
			while(Temperature==Temperature_Lastchange || Humidity == Humidity_Lastchange) pthread_cond_wait(&dht11_cond,&dht11_mutex_temp);	//cho dieu kien
			snprintf(message, 100, "tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, Humidity);
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
			pthread_mutex_unlock(&dht11_mutex_temp);//mo khoa mutex
		}
		
		
	}
}


void *func_thread_uart_transmit(void *ptr){
	int ret, fd;
	char temp[64];
	while(1){
		pthread_mutex_lock(&uart_mutex);
		while(!send_check) pthread_cond_wait(&uart_cond, &uart_mutex);
		printf("Starting open uart device\n");
		//O_NOCTTY: No control tty, O_NDELAY: No delay
		fd = open("/dev/ttyAMA0", O_RDWR|O_NOCTTY|O_NDELAY);
		if(fd<0){
			perror("Falied to open uart\n");
			return errno;
		}

		//Trong qua trinh ghi tat viec doc cua cac process khac bang khoa fcntl
		//Turn off blocking for read
		fcntl(fd, F_SETFL, 0);

		ret = write(fd, message_uart_send, strlen(message_uart_send));
		if(ret < 0){
			perror("Failed to write uart\n");
			return errno;
		}
		pthread_mutex_unlock(&uart_mutex);
	}
}
void *func_thread_uart_receive(void *ptr){
	int ret, fd;
	char temp[64];
	while(1){
		pthread_mutex_lock(&uart_mutex);
		fd = open("/dev/ttyAMA0", O_RDWR|O_NOCTTY|O_NDELAY);
		if(fd<0){
			perror("Failed to open uart file\n");
			return errno;
		}
		ret = read(fd, (char *)message_uart_receive, 64);
		if(ret<0){
			perror("Cannot read uart from uart device file\n");
			send_check=0;
			return errno;
		}
		else if(ret==0){
			printf("No data on port\n");
			send_check=0;
		}
		else{
			message_uart_receive[ret]='\0';
			printf("%i bytes read: %s\n", ret, message_uart_receive);
			send_check=1;
			pthread_cond_signal(&uart_cond);
		}
		pthread_mutex_unlock(&uart_mutex);
	}
}