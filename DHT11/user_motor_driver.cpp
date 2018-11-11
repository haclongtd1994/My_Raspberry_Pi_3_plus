#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <curl/curl.h>
#include <pthread.h>

#define RED			"\x1B[31m"
#define GRN			"\x1B[32m"
#define YEL			"\x1B[33m"
#define BLU			"\x1B[34m"
#define MAG			"\x1B[35m"
#define CYN			"\x1B[36m"
#define WHY			"\x1B[37m"
#define RESET		"\x1B[0m"

#define WEB_DEVICE	"https://luutruthietbiiot.000webhostapp.com/receive_device_id.php"

static 	int 		Humidity, Temperature;//using for function dht11
pthread_mutex_t		dht11_mutex 	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		dht11_cond		= PTHREAD_COND_INITIALIZER;
static int send_hum=0,send_temp=0;

static 	char 		user_device[]="raspberry";
static 	char 		password_device[]="hunglapro13";

void *func_thread_curl_dht11_humidity(void *ptr);
void *func_thread_curl_dht11_temperature(void *ptr);
void *func_thread_dht11(void *ptr);


int main(void){
	pthread_t thread_dht11;
	pthread_t thread_curl_dht11_humidity;
	pthread_t thread_curl_dht11_temperature;

	int iret_curl_dht11_humidity;
	int iret_curl_dht11_temperature;
	int iret_dht11;

	printf("\r\n" YEL "Program user with curl and motor control" RESET "\r\n");

	curl_global_init(CURL_GLOBAL_ALL);

	iret_dht11 = pthread_create(&thread_dht11, NULL, func_thread_dht11, NULL);
	iret_curl_dht11_humidity = pthread_create(&thread_curl_dht11_humidity, NULL, func_thread_curl_dht11_humidity, NULL);
	iret_curl_dht11_temperature = pthread_create(&thread_curl_dht11_temperature, NULL, func_thread_curl_dht11_temperature, NULL);

	printf("Ket qua luong gui du lieu dht11 humidity: %d\r\n", iret_curl_dht11_humidity);
	printf("Ket qua luong gui du lieu dht11 temperature %d\r\n", iret_curl_dht11_temperature);
	printf("Ket qau luong doc du lieu dht11: %d\r\n", iret_dht11);

	pthread_join(thread_dht11, NULL);
	pthread_join(thread_curl_dht11_humidity, NULL);
	pthread_join(thread_curl_dht11_temperature, NULL);

	curl_global_cleanup();
	return 0;
}
void *func_thread_dht11(void *ptr){
	int ret,fd;
	int dht11_data[4]={0};
	int Humidity_Lastchange=0;
	int Temperature_Lastchange=0;

	printf("Starting device dht11 test ... \n");
	fd = open("/dev/DHT11", O_RDWR);
	if(fd<0){
		printf("Failed to open device DHT11\n");
		return errno;
	}
	while(1){
		send_hum = send_temp =0;
		pthread_mutex_lock(&dht11_mutex);
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
		printf("Check Humidity: %d, Temperature: %d\r\n", Humidity_Lastchange, Temperature_Lastchange);
		pthread_mutex_unlock(&dht11_mutex);
		if(Temperature!=Temperature_Lastchange || Humidity != Humidity_Lastchange){
			printf("Call condition variable\n");
			Temperature_Lastchange = Temperature;
			Humidity_Lastchange = Humidity;
			while(send_temp==0||send_hum==0) pthread_cond_broadcast(&dht11_cond);
		}
	}
}

void *func_thread_curl_dht11_humidity(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "Humidity";

	while(1){
		pthread_mutex_lock(&dht11_mutex);//giu khoa mutex
		while(send_hum==1) pthread_cond_wait(&dht11_cond,&dht11_mutex);	//cho dieu kien
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
		pthread_mutex_unlock(&dht11_mutex);//mo khoa mutex
		send_hum=1;
	}
}
void *func_thread_curl_dht11_temperature(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "Temperature";

	while(1){
		pthread_mutex_lock(&dht11_mutex);
		while(send_temp==1) pthread_cond_wait(&dht11_cond,&dht11_mutex);
		snprintf(message, 100, "tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, Temperature);
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
		pthread_mutex_unlock(&dht11_mutex);
		send_temp=1;
	}
}