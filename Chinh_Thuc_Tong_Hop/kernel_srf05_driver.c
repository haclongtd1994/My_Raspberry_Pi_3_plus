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
#include <linux/ktime.h>
#include <linux/interrupt.h>


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

#define SRF05_PIN_T			24
#define SRF05_PIN			23
#define HIGH				1
#define LOW					0

static int major_number;
static int numberOpens=0;
static char message[256]={0};
static short size_of_message;
static struct device *srf05_device;
static struct class *srf05_class;
static long long int srf05_data[3]={0,0,0};

static short int irq_gpio;
static int temp=0;
static int OK=0;
static int valid_value = 0;
static ktime_t echo_start;
static ktime_t echo_end;

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
static void GPIO_PULL(u32 value)
{
	iowrite32(value, (u32 *)gpio_base + 37);
}

static void GPIO_PULLCLK0(u32 clock)
{
	iowrite32(clock, (u32 *)gpio_base + 38);
}

static void receive_data_srf05(void){
	int counter, to;
	while(!OK){
		to=0;
		//triger and delay 10us
		SET_PIN_LOW(SRF05_PIN_T);
		mdelay(200);
		SET_PIN_HIGH(SRF05_PIN_T);
		udelay(10);
		SET_PIN_LOW(SRF05_PIN_T);
		//change input and while to mearsuring SRF05
		
		counter=0;
		while (valid_value==0) {
			// Out of range
			if (++counter>23200) {
				printk("Time out");to=1;break;
			}
			udelay(1);
		}

		srf05_data[0] =  ktime_to_ns(echo_start);
		srf05_data[1] =  ktime_to_ns(echo_end);
		srf05_data[2] = ktime_to_ns(ktime_sub(echo_end,echo_start));
		printk(KERN_INFO"SRF05_DATA: %lld", srf05_data[2]);
		if(to==1)  	OK=0;
		else		OK=1;
	}
	printk(KERN_INFO"Complete mearsuring\n");
}

static irqreturn_t  button_irq_handler(int irq, void *dev_id)
{
	ktime_t ktime_dummy;

	//gpio_set_value(HCSR04_TEST,1);

	if (valid_value==0) {
		ktime_dummy=ktime_get();
		if (READ_DATA_PIN(SRF05_PIN)==1) {
			echo_start=ktime_dummy;
		} else {
			echo_end=ktime_dummy;
			valid_value=1;
		}
	}

	//gpio_set_value(HCSR04_TEST,0);
	return IRQ_HANDLED;
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


	SET_PIN_OUTPUT(SRF05_PIN_T);
	SET_PIN_INPUT(SRF05_PIN);
	GPIO_PULL(2);
	GPIO_PULLCLK0(0x00800000);

	irq_gpio = gpio_to_irq(SRF05_PIN);
	if (irq_gpio < 0) {
		printk(KERN_INFO "%s: GPIO to IRQ mapping failure\n", __func__);
		return -ENODEV;
	}

	printk(KERN_INFO "%s: Mapped interrupt %d\n", __func__, irq_gpio);

	if (request_irq(irq_gpio,
		button_irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
		"hy-srf05",
		NULL)) {
		printk(KERN_INFO "%s: Irq request failure\n", __func__);
		return -ENODEV;
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
		srf05_data[0] = 0;
		receive_data_srf05();
	}
	printk(KERN_INFO"Receive message from user: %s\n", message);
	size_of_message = strlen(message);
	return len;
}
static ssize_t srf05_read(struct file* filep, char *buf, size_t len, loff_t *offset){
	int error;
	if(OK){
		printk(KERN_INFO "message to user: MUC NUOC: %lld\n", srf05_data[2]);
		error = copy_to_user(buf, srf05_data, sizeof(long long int)*3);
		if(!error){
			printk(KERN_INFO "Send OK: MUC NUOC: %lld\n", srf05_data[2]);
			return 32;
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