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
pthread_cond_t		dht11_cond_1	= PTHREAD_COND_INITIALIZER;
int check1=0, check2=0;

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
	while(1){
		check1 = check2 = 0;
		
		sleep(0.2);
		printf("Call condition variable\n");
		sleep(0.2);
		
		pthread_mutex_lock(&dht11_mutex);
		pthread_cond_broadcast(&dht11_cond);
		pthread_mutex_unlock(&dht11_mutex);
	}
}

void *func_thread_curl_dht11_humidity(void *ptr){
	while(1){
		pthread_mutex_lock(&dht11_mutex);
		pthread_cond_wait(&dht11_cond,&dht11_mutex);
		pthread_mutex_unlock(&dht11_mutex);
		printf("Call function 2\n");
		sleep(0.2);
		
		check1=1;
	}
}
void *func_thread_curl_dht11_temperature(void *ptr){

	while(1){
		pthread_mutex_lock(&dht11_mutex);
		 pthread_cond_wait(&dht11_cond,&dht11_mutex);
		pthread_mutex_unlock(&dht11_mutex);
		printf("Call function 3\n");
		sleep(0.2);
		check2=1;
	}
}