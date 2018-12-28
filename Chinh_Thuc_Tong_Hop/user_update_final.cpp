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

#define BUFFER		256
static 	char 		receive[BUFFER];//using for function motor
static 	int 		Humidity=0, Temperature=0;//using for function dht11
int Humidity_Lastchange=30;
int Temperature_Lastchange=30;
int send_dht11_T=0;
int send_dht11_H=0;
int send_mini_uart = 0;

pthread_mutex_t		dht11_mutex_T 	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t		dht11_mutex_H 	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		dht11_cond_H		= PTHREAD_COND_INITIALIZER;
pthread_cond_t		dht11_cond_T		= PTHREAD_COND_INITIALIZER;
pthread_mutex_t		mini_uart_mutex 	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		mini_uart_cond		= PTHREAD_COND_INITIALIZER;

static 	int 		data=0;
static 	int 		data_gpio=0;
static 	char 		user_device[]="raspberry";
static 	char 		password_device[]="hunglapro13";


void *func_thread_curl_gpio(void *ptr);
void *func_thread_gpio(void *ptr);
void *func_thread_curl_dht11_humidity(void *ptr);
void *func_thread_curl_dht11_temperature(void *ptr);
void *func_thread_dht11(void *ptr);
void *func_thread_mini_uart_send(void *ptr);
void *func_thread_mini_uart_receive(void *ptr);


struct Check_Status_Tang_2{
	bool gpio_1 = false;
	bool gpio_2 = false;
};
Check_Status_Tang_2 check;

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

	int iret_curl_gpio;
	int iret_gpio;
	int iret_curl_dht11_humidity;
	int iret_curl_dht11_temperature;
	int iret_dht11;
	int iret_mini_uart_send;
	int iret_mini_uart_receive;

	printf("\r\n" YEL "Program user with curl and motor control" RESET "\r\n");

	curl_global_init(CURL_GLOBAL_ALL);

	iret_curl_gpio = pthread_create(&thread_curl_gpio, NULL, func_thread_curl_gpio, NULL);
	iret_gpio = pthread_create(&thread_gpio, NULL, func_thread_gpio, NULL);
	iret_dht11 = pthread_create(&thread_dht11, NULL, func_thread_dht11, NULL);
	iret_curl_dht11_humidity = pthread_create(&thread_curl_dht11_humidity, NULL, func_thread_curl_dht11_humidity, NULL);
	iret_curl_dht11_temperature = pthread_create(&thread_curl_dht11_temperature, NULL, func_thread_curl_dht11_temperature, NULL);
	iret_mini_uart_send = pthread_create(&thread_mini_uart_send, NULL, func_thread_mini_uart_send, NULL);
	iret_mini_uart_receive = pthread_create(&thread_mini_uart_receive, NULL, func_thread_mini_uart_receive, NULL);

	printf("Ket qua luong gui du lieu: %d\r\n",iret_curl_gpio);
	printf("Ket qua luong dieu khien gpio: %d\r\n",iret_gpio);
	printf("Ket qua luong gui du lieu dht11 humidity: %d\r\n", iret_curl_dht11_humidity);
	printf("Ket qua luong gui du lieu dht11 temperature: %d\r\n", iret_curl_dht11_temperature);
	printf("Ket qau luong doc du lieu dht11: %d\r\n", iret_dht11);
	printf("Ket qau luong doc du lieu mini uart: %d\r\n", iret_mini_uart_send);
	printf("Ket qau luong doc du lieu mini uart: %d\r\n", iret_mini_uart_receive);

	pthread_join(thread_gpio, NULL);
	pthread_join(thread_curl_gpio,NULL);
	pthread_join(thread_dht11, NULL);
	pthread_join(thread_curl_dht11_humidity, NULL);
	pthread_join(thread_curl_dht11_temperature, NULL);
	pthread_join(thread_mini_uart_send, NULL);
	pthread_join(thread_mini_uart_receive, NULL);

	curl_global_cleanup();
	return 0;
}


void *func_thread_mini_uart_receive(void *ptr){
	int fd;
	char buf[256];
	int n;
	// Open the Port. We want read/write, no "controlling tty" status, and open it no matter what state DCD is in
	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		perror("open_port: Unable to open /dev/ttyS0 - ");
		return(-1);
	}
	fcntl(fd, F_SETFL, 0);
	while(true){
		n = read(fd, (void*)buf, 255);
		if (n < 0) {
			perror("Read failed - ");
			return -1;
		}
		else if (n == 0) printf("No data on port\n");
		else {
			buf[n] = '\0';
			printf("%i bytes read : %s", n, buf);
		}
		pthread_mutex_lock(&mini_uart_mutex);
		send_mini_uart=1;
		pthread_cond_signal(&mini_uart_cond);
		pthread_mutex_unlock(&mini_uart_mutex);
	}
}


void *func_thread_mini_uart_send(void *ptr){
	int fd;
	char mini_uart_send[64]="\0";
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
  		send_mini_uart = 0;
  			fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
		  	if (fd == -1) {
		   	 	perror("open_port: Unable to open /dev/ttyS0 - ");
		    	return(-1);
		  	}
  			// Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
		  	

		  	snprintf(mini_uart_send, 64, "Dht11: 12 28, Gpio: 1\r\n");
		  	printf("%s\n", mini_uart_send);
		  	// Write to the port
		  	int n = write(fd,mini_uart_send, strlen(mini_uart_send));
		  	if (n < 0) {
		    	perror("Write failed - ");
		    	return -1;
		  	}
		
  	}
} 

void *func_thread_curl_gpio(void *ptr){
	CURL *curl_handle;
	CURLcode res;

	char message[100];
	int frame_number = 0;
	char devSerialNumber[] = "GPIO";

	while(1){
		snprintf(message, 100, "tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, data_gpio);
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
		snprintf(message, 100, "tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, Humidity);
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
		snprintf(message, 100, "tk=%s&mk=%s&serialnumber=%s&frame=%d&data=%d",user_device, password_device,devSerialNumber, frame_number, Temperature);
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