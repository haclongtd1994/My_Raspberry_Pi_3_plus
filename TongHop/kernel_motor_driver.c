#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/mutex.h>

MODULE_AUTHOR("Phan Thanh Hung");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Module to control motor from user space IPC with kernel thourgh by file system");

static void __iomem *gpio_base;
#define GPIO_ADD_BASE	0x3F200000
#define MOTOR_PIN		17
#define GPIO_SET_0		7
#define GPIO_CLR_0		10

#define DEVICE_NAME		"motor_test"
#define CLASS_NAME	"test1"

static DEFINE_MUTEX(motor_mutex);

static int major_number;
static struct class *motor_class;
static struct device *motor_device;
static int numberOpens=0;
static char message[256]={0};
static short size_of_message;

static int motor_open(struct inode*, struct file*);
static int motor_release(struct inode*, struct file*);
static ssize_t motor_read(struct file*, char *, size_t, loff_t *);
static ssize_t motor_write(struct file*, const char *, size_t, loff_t *);


static struct file_operations fops={
	.open = motor_open,
	.read = motor_read,
	.write = motor_write,
	.release = motor_release,
};


static void SET_PIN_OUTPUT(u32 pin){
	u32 value=0;
	u32 *address = NULL;
	if(pin>40){
		printk(KERN_ALERT "Pin not found\n");
		return;
	}
	address = (u32 *)gpio_base + pin/10;
	value = ioread32(address);
	value |= (1<<((pin%10)*3));
	iowrite32(value,address);
}

static void SET_PIN_HIGH(u32 pin){
	if(pin>40){
		printk(KERN_ALERT "Pin not found\n");
		return;
	}
	iowrite32(1<<pin,(u32 *)gpio_base + GPIO_SET_0);
}
static void SET_PIN_LOW(u32 pin){
	if(pin>40){
		printk(KERN_ALERT "Pin not found\n");
		return;
	}
	iowrite32(1<<pin,(u32 *)gpio_base + GPIO_CLR_0);
}

static int __init motor_init(void){
	printk(KERN_INFO "Init of motor module start\n");
	gpio_base = ioremap(GPIO_ADD_BASE,0x100);
	if(gpio_base == NULL){
		printk(KERN_ALERT "Map gpio phys to vitrual mem failed\n");
		return -ENOMEM;
	}
	printk(KERN_INFO "Map gpio successfull\n");


	//init motor
	SET_PIN_OUTPUT(MOTOR_PIN);
	SET_PIN_LOW(MOTOR_PIN);

	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if(major_number<0){
		printk(KERN_ALERT "Failed to register device\n");
		return major_number;
	}
	printk(KERN_INFO "Register success\n");

	motor_class = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(motor_class)){
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create class device\n");
		return PTR_ERR(motor_class);
	}

	motor_device = device_create(motor_class, NULL, MKDEV(major_number,0), NULL, DEVICE_NAME);
	if(IS_ERR(motor_device)){
		class_destroy(motor_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create device\n");
		return PTR_ERR(motor_device);
	}
	mutex_init(&motor_mutex);
	printk(KERN_INFO "Devicce create correcly\n");
	return 0;
}
static void __exit motor_exit(void){
	mutex_destroy(&motor_mutex);
	device_destroy(motor_class, MKDEV(major_number,0));
	class_unregister(motor_class);
	class_destroy(motor_class);
	unregister_chrdev(major_number, DEVICE_NAME);
	printk(KERN_INFO "Module exit. Good bye! \n");
}

static int motor_open(struct inode *inodep, struct file* filep){
	if(!mutex_trylock(&motor_mutex)){
		printk(KERN_ALERT "Module is used by another process");
		return -EBUSY;
	}
	numberOpens++;
	printk(KERN_INFO "User have open : %d", numberOpens);
	return 0;
}
static int motor_release(struct inode *inodep, struct file* filep){
	mutex_unlock(&motor_mutex);
	printk(KERN_INFO "Module closed\n");
	return 0;
}
static ssize_t motor_read(struct file* filep, char *buf, size_t len, loff_t *offset){
	int error;
	error = copy_to_user(buf,message,size_of_message);
	if(!error){
		printk(KERN_INFO "Send to user %d character\n",size_of_message);
		return size_of_message;
	}
	else{
		printk(KERN_ALERT "Failed to send to user space with error: %d",error);
		return -EFAULT;
	}
}
static ssize_t motor_write(struct file *filep, const char *buf, size_t len, loff_t *offset){
	sprintf(message,"%s",buf);
	size_of_message = strlen(message);
	if(!strcmp(message,"ON")){
		SET_PIN_HIGH(MOTOR_PIN);
	}
	else if(!strcmp(message,"OFF")){
		SET_PIN_LOW(MOTOR_PIN);
	}
	printk(KERN_INFO "Recive message from user %zu character\n",len);
	return len;
}


module_init(motor_init);
module_exit(motor_exit);