#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>

MODULE_AUTHOR("Phan Thanh Hung");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Module to control gpio from user space IPC with kernel thourgh by file system");

#define GPIO_ADD_BASE	0x3F200000
#define GPIO_SET_0		7
#define GPIO_CLR_0		10
#define GPIO_LVL_0		13

#define DEVICE_NAME		"button_1"
#define CLASS_NAME		"datn_button_1"

static int major_number;
static struct class *class_device;
static struct device *device_driver;
static u32 BUTTON_PIN = 23;
static int OK=0;

static DEFINE_MUTEX(mutex_gpio);
static void __iomem *gpio_base;
static int numberOpens=0, state=0;
static short size_of_message;
static char message[256]={0};

static int gpio_open(struct inode*, struct file *);
static int gpio_release(struct inode*, struct file *);
static ssize_t	gpio_read(struct file*, char *, size_t, loff_t *);
static ssize_t gpio_write(struct file*, const char *, size_t, loff_t *);

static struct file_operations fops={
	.open = gpio_open,
	.read = gpio_read,
	.write = gpio_write,
	.release = gpio_release,
};

static void GPIO_PULL(u32 value)
{
	iowrite32(value, (u32 *)gpio_base + 37);
}

static void GPIO_PULLCLK0(u32 clock)
{
	iowrite32(clock, (u32 *)gpio_base + 38);
}

static void SET_PIN_INPUT(u32 pin){
	u32 value=0;
	u32 *address = NULL;
	if(pin>40){
		printk(KERN_ALERT "Not found\n");
		return;
	}
	address = (u32 *)gpio_base + pin/10;
	value = ioread32(address);
	value &= ~(7<<((pin%10)*3));
	iowrite32(value, address);
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

static int __init gpio_init(void){
	printk(KERN_INFO "Module gpio init to start\n");
	gpio_base = ioremap(GPIO_ADD_BASE,0x100);
	if(gpio_base == NULL){
		printk(KERN_ALERT "Can not map gpio phys to vitrual mem");
		return -ENOMEM;
	}
	OK=0;
	//Init gpio with default
	SET_PIN_INPUT(BUTTON_PIN);
	GPIO_PULL(2);
	GPIO_PULLCLK0(0x01000000);

	printk(KERN_INFO "Start to register chardev\n");
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if(major_number<0){
		printk(KERN_ALERT "Can not register major_number\n");
		return major_number;
	}
	printk(KERN_INFO "Register success with major_number: %d",major_number);

	class_device = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(class_device)){
		unregister_chrdev(major_number,DEVICE_NAME);
		printk(KERN_ALERT "Register class device failed");
		return PTR_ERR(class_device);
	}
	printk(KERN_INFO "Register class device success\n");

	device_driver = device_create(class_device, NULL, MKDEV(major_number,0),NULL, DEVICE_NAME);
	if(IS_ERR(device_driver)){
		class_destroy(class_device);
		unregister_chrdev(major_number,DEVICE_NAME);
		printk(KERN_ALERT "Register class device failed");
		return PTR_ERR(device_driver);
	}
	printk(KERN_INFO "Register device driver for gpio correct\n");
	mutex_init(&mutex_gpio);
	return 0;

}
static void __exit gpio_exit(void){
	mutex_destroy(&mutex_gpio);
	device_destroy(class_device, MKDEV(major_number,0));
	class_unregister(class_device);
	class_destroy(class_device);
	unregister_chrdev(major_number,DEVICE_NAME);
	printk(KERN_INFO "Module exit\n");
}

static int gpio_open(struct inode *inodep, struct file *ffilep){
	if(!mutex_trylock(&mutex_gpio)){
		printk(KERN_ALERT "GPIO device is using by another process\n");
		return -EBUSY;
	}
	numberOpens++;
	printk(KERN_INFO "Open gpio device driver with number opens: %d\n",numberOpens);
	return 0;
}
static int gpio_release(struct inode *inodep, struct file *filep){
	mutex_unlock(&mutex_gpio);
	printk(KERN_INFO "Gpio device driver is closing\n");
	return 0;
}
static ssize_t gpio_read(struct file *filep, char *buf, size_t len, loff_t *offset){
	int error =0;		
	sprintf(message,"%d",state);
	error = copy_to_user(buf, message,size_of_message);
	if(!error){
		printk(KERN_INFO "Send %s to user\n", message);
		return size_of_message;
	}
	else{
		printk(KERN_ALERT "Can not send message to user\n");
		return error;
	}
}
static ssize_t gpio_write(struct file *filep, const char *buf, size_t len, loff_t *offset){
	sprintf(message,"%s",buf);
	if(!strcmp(message,"ON")){
		if(READ_DATA_PIN(BUTTON_PIN))		state=1;
		else								state=0;
	}
	printk(KERN_INFO "Receive %s from user\n",message);
	size_of_message = strlen(message);
	return len;
}

module_init(gpio_init);
module_exit(gpio_exit);