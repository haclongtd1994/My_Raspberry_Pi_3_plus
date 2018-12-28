#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/time.h>

MODULE_AUTHOR("PTH");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("SRF05 ULTRA SONIC");
#define DEVICE_NAME "SRF05"
#define CLASS_NAME	"datn_srf05"

static DEFINE_MUTEX(mutex_srf05);

static void __iomem *gpio_base;
#define PBASE				0x3f000000
#define BASE_ADDR_GPIO		(PBASE+0x200000)
#define GPIO_SET_0			7
#define GPIO_CLR_0			10
#define GPIO_LVL_0			13
#define GPIO_PUD			37
#define GPIO_PUD_CLK		38

#define SRF05_PIN			26
#define HIGH				1
#define LOW					0

static int major_number;
static int numberOpens=0;
static char message[256]={0};
static short size_of_message;
static struct device *srf05_device;
static struct class *srf05_class;
static int srf05_data=0;
static int OK=0;

static int srf05_open(struct inode*, struct file*);
static int srf05_release(struct inode*, struct file*);
static ssize_t srf05_write(struct file*, const char *, size_t, loff_t *);
static ssize_t srf05_read(struct file*, char*, size_t, loff_t*);

static struct file_operations fops={
	.open = srf05_open,
	.read = srf05_read,
	.write = srf05_write,
	.release = srf05_release,
	.owner = THIS_MODULE
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

static void receive_data_srf05(void){
	struct timeval t0,t1;
	//triger and delay 10us
	SET_PIN_OUTPUT(SRF05_PIN);
	SET_PIN_HIGH(SRF05_PIN);
	udelay(10);
	SET_PIN_LOW(SRF05_PIN);
	//change input and while to mearsuring SRF05
	SET_PIN_INPUT(SRF05_PIN);
	while(!READ_DATA_PIN(SRF05_PIN));
	do_gettimeofday(&t0);
	while(READ_DATA_PIN(SRF05_PIN));
	do_gettimeofday(&t1);

	srf05_data = (unsigned long) (t1.tv_sec - t0.tv_sec)*1000 + (t1.tv_usec - t0.tv_usec)/1000;
	srf05_data = srf05_data/58;
	OK=1;
	printk(KERN_INFO"Complete mearsuring\n");
}


static int __init srf05_init(void){
	printk(KERN_INFO"Starting config SRF05___Init....\n");
	gpio_base = ioremap(BASE_ADDR_GPIO,0x100);
	if(gpio_base==NULL){
		printk(KERN_ALERT"Cannot allocate mem for srf05\n");
		return -ENOMEM;
	}
	//Register major number
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if(major_number<0){
		printk(KERN_ALERT"Cannot register major_number\n");
		return major_number;
	}
	srf05_class = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(srf05_class)){
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT"Cannot create class for device\n");
		return PTR_ERR(srf05_class);
	}
	srf05_device = device_create(srf05_class, NULL, MKDEV(major_number,0), NULL, DEVICE_NAME);
	if(IS_ERR(srf05_class)){
		class_destroy(srf05_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT"Cannot create device file for srf05\n");
		return PTR_ERR(srf05_device);
	}
	mutex_init(&mutex_srf05);
	printk(KERN_INFO"Create device file for SRF05 complete\n");
	return 0;

}
static void __exit srf05_exit(void){
	iounmap(gpio_base);
	mutex_destroy(&mutex_srf05);
	device_destroy(srf05_class, MKDEV(major_number,0));
	class_unregister(srf05_class);
	class_destroy(srf05_class);
	unregister_chrdev(major_number, DEVICE_NAME);
	printk("Module SRF05 Exiting....\n");
}

static int srf05_open(struct inode* inodep, struct file* filep){
	if(!mutex_trylock(&mutex_srf05)){
		printk(KERN_ALERT"Module is using by another process\n");
		return -EBUSY;
	}
	numberOpens++;OK=0;
	printk(KERN_INFO"OpenNumbers: %d\n",numberOpens);
	return 0;
}
static int srf05_release(struct inode* inodep, struct file* filep){
	mutex_unlock(&mutex_srf05);
	printk(KERN_INFO"Module is closing...\n");
	return 0;
}
static ssize_t srf05_write(struct file* filep, const char *buf, size_t len, loff_t *offset){
	sprintf(message,"%s",buf);
	if(!strcmp(message,"READ_SRF05")){
		receive_data_srf05();
	}
	printk(KERN_INFO"Receive message from user: %s\n", message);
	size_of_message = strlen(message);
	return len;
}
static ssize_t srf05_read(struct file* filep, char *buf, size_t len, loff_t *offset){
	int error;
	if(OK){
		error = copy_to_user(buf, &srf05_data, 4);
		if(!error){
			printk(KERN_INFO "message to user: MUC NUOC: %d\n", srf05_data);
			return 4;
		}
		else{
			printk(KERN_ALERT "Cannot send message to user \n");
			return error;
		}
	}
	else return OK;
}
module_init(srf05_init);
module_exit(srf05_exit);