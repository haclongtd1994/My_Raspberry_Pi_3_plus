#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/delay.h>

MODULE_AUTHOR("PTH");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("DHT11 FOR RASPBERRY PI 3+");
MODULE_LICENSE("GPL");
#define DEVICE_NAME		"DHT11"
#define CLASS_NAME		"datn_dht11"

static DEFINE_MUTEX(mutex_dht11);

static void __iomem *gpio_base;
#define PBASE			0x3f000000
#define BASE_ADDR_GPIO	(PBASE + 0x200000)
#define GPIO_SET_0		7
#define GPIO_CLR_0		10
#define GPIO_LVL_0		13
#define GPIO_PUD		37
#define GPIO_PUD_CLK	38

#define DHT11_PIN		27
#define MAXTIMINGS		85
#define HIGH 			1
#define LOW 			0
static int dht11_data[5]={0,0,0,0,0};
static int OK=0;

static int major_number;
static int numberOpens=0;
static char message[256]={0};
static short size_of_message;
static struct device *dht11_device;
static struct class *dht11_class;

static int dht11_open(struct inode*, struct file*);
static int dht11_release(struct inode*, struct file*);
static ssize_t dht11_read(struct file*, char *, size_t, loff_t *);
static ssize_t dht11_write(struct file*, const char *, size_t, loff_t *);


static struct file_operations fops={
	.open = dht11_open,
	.read = dht11_read,
	.write = dht11_write,
	.release = dht11_release
};
static void SET_PIN_OUTPUT(u32 pin){
	u32 value;
	u32 *address=NULL;
	if(pin>40){
		printk(KERN_ALERT "Cannot found\n");
		return;
	}
	address = (u32 *)gpio_base + pin/10;
	value = ioread32(address);
	value &= ~(0x07<<(pin%10)*3);	//xoa cac bit lien quan ve 0 set ve input
	value |= (1<<(pin%10)*3);		//set lai output
	iowrite32(value, address);
}
static void SET_PIN_INPUT(u32 pin){
	u32 value;
	u32 *address=NULL;
	if(pin>40){
		printk(KERN_ALERT "Cannot found\n");
		return;
	}
	address = (u32 *)gpio_base + pin/10;
	value = ioread32(address);
	value &= ~(0x07<<(pin%10)*3);	//xoa cac bit lien quan ve 0
	iowrite32(value, address);
}
static void SET_PIN_HIGH(u32 pin){
	if(pin>40){
		printk(KERN_ALERT "Cannot found\n");
		return;
	}
	iowrite32(1<<pin, (u32 *)gpio_base + GPIO_SET_0);
}
static void SET_PIN_LOW(u32 pin){
	if(pin>40){
		printk(KERN_ALERT "Cannot found\n");
		return;
	}
	iowrite32(1<<pin, (u32 *)gpio_base + GPIO_CLR_0);
}

//check pin data 1: high, 0: low, 5: error
static int READ_DATA_PIN(u32 pin){
	u32 value;
	u32 *address=NULL;
	if(pin>40){
		printk(KERN_ALERT "Not found\n");
		return 5;	//failed to check data pin
	}
	address = (u32 *)gpio_base + GPIO_LVL_0;
	value = ioread32(address);
	value &= (1<<pin);
	value = value >> pin;
	if(value == 0x01) return 1;
	else return 0;
}

static void gpio_init_for_dht11(void){
	SET_PIN_OUTPUT(DHT11_PIN);
	SET_PIN_HIGH(DHT11_PIN);
}

//MAXTIMING=85 vi :
// 5 byte = 8*5 = 40 bit, moi bit gui kem theo 1 bit data thap -> 40*2 = 80bit, bo 3 bit dau -> 83bit cong them 2 bit de neu co loi se gui 2 bit bu vao
static void receive_data_dht11(void){
	u8 laststate =HIGH;
	u8 counter=0;
	u8 j=0,i;

	dht11_data[0] = dht11_data[1] = dht11_data[2] = dht11_data[3] = dht11_data[4] = 0;
	//send start signal to dht11 delay 18us
	SET_PIN_OUTPUT(DHT11_PIN);
	SET_PIN_LOW(DHT11_PIN);
	mdelay(18);
	//Pull up MCU to high 40us
	SET_PIN_HIGH(DHT11_PIN);
	udelay(40);
	//Set pin to input to receive data
	SET_PIN_INPUT(DHT11_PIN);
	for(i=0;i<MAXTIMINGS;i++){
		counter = 0;
		while(READ_DATA_PIN(DHT11_PIN) == laststate){
			counter++;
			udelay(1);
			if(counter == 255) break;
		}
		laststate = READ_DATA_PIN(DHT11_PIN);
		if(counter == 255) break;
		if((i>=4)&&(i%2==0)){
			dht11_data[j/8]	= dht11_data[j/8]<<1;
			if(counter > 26)	dht11_data[j/8] |= 1;
			j++;
		}
	}
	//check phai gui hon 40 bit hop le va phai check sum lai voi mang thu 4
	if((j>=40)&&(dht11_data[4] = (dht11_data[0] + dht11_data[1] + dht11_data[2] + dht11_data[3]) & 0xff)){
		OK=1;
	}
	else{
		OK=0;
		printk(KERN_ALERT "Data not good\n");
	}
}


static int __init dht11_init(void){
	printk(KERN_INFO "Start Init config DHT11\n");
	gpio_base = ioremap(BASE_ADDR_GPIO, 0x100);
	if(gpio_base == NULL){
		printk(KERN_ALERT "Can not map gpio to vitrual mem\n");
		return -ENOMEM;
	}
	//init gpio
	gpio_init_for_dht11();

	//major_number
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if(major_number<0){
		printk(KERN_ALERT "Cannot register major number\n");
		return major_number;
	}
	printk(KERN_INFO "%s:Register major_number successfull: %d", __func__, major_number);

	//create class for device
	dht11_class = class_create(THIS_MODULE,CLASS_NAME);
	if(IS_ERR(dht11_class)){
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Cannot create class for device\n");
		return PTR_ERR(dht11_class);
	}
	printk(KERN_INFO "Class device created\n");

	//create device
	dht11_device = device_create(dht11_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
	if(IS_ERR(dht11_device)){
		class_destroy(dht11_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Cannot create device file\n");
		return PTR_ERR(dht11_device);
	}
	mutex_init(&mutex_dht11);
	printk(KERN_INFO "Create device file complete\n");
	return 0;
}
static void __exit dht11_exit(void){
	mutex_destroy(&mutex_dht11);
	device_destroy(dht11_class, MKDEV(major_number, 0));
	class_unregister(dht11_class);
	class_destroy(dht11_class);
	unregister_chrdev(major_number, DEVICE_NAME);
	printk(KERN_INFO "Module is exiting \n");
}


static int dht11_open(struct inode *inodep, struct file* filep){
	if(!mutex_trylock(&mutex_dht11)){
		printk(KERN_ALERT "Module is using by another process\n");
		return -EBUSY;
	}
	numberOpens++;
	printk(KERN_INFO "Module is opening with open numbers:%d\n",numberOpens);
	return 0;
}
static int dht11_release(struct inode *inodep, struct file* filep){
	mutex_unlock(&mutex_dht11);
	printk(KERN_INFO "Module is closing\n");
	return 0;
}
static ssize_t dht11_read(struct file* filep, char *buf, size_t len, loff_t *offset){
	int error;
	if(OK==1){
		error = copy_to_user(buf, dht11_data, 16);
		if(!error){
			printk(KERN_INFO "message to user: HuM: %d.%d %% Temp: %d.%d *C\n", dht11_data[0], dht11_data[1], dht11_data[2], dht11_data[3]);
			return 4;
		}
		else{
			printk(KERN_ALERT "Cannot send message to user \n");
			return error;
		}
	}
	else return OK;
}
static ssize_t dht11_write(struct file *filep, const char *buf, size_t len, loff_t *offset){
	sprintf(message, "%s", buf);
	if(!strcmp(message, "READ_DHT11")){
		receive_data_dht11();
	}
	printk(KERN_INFO "Receive message from user: %s\n", message);
	size_of_message = strlen(message);
	return len;
}

module_init(dht11_init);
module_exit(dht11_exit);