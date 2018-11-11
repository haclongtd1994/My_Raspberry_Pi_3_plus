#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/delay.h>

MODULE_AUTHOR("PHAN THANH HUNG");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("UART DRIVER FOR KERNEL SPACE");
MODULE_LICENSE("GPL");
#define DEVICE_NAME		"uart_test_kernel"
#define CLASS_NAME		"uart_test"
//peripherals base add : see https://www.raspberrypi.org/documentation/hardware/raspberrypi/peripheral_addresses.md
#define PBASE			0x3F000000
//gpio register _ reference manual - page 90,5 			//			1 register: 4byte -> 7x4 = 28 -> hex: 1C
#define BASE_ADD_GPIO	(PBASE + 0x200000)
#define GPIO_SET_0		7
#define GPIO_CLR_0		10
#define GPIO_LVL_0		13
#define GPIO_PUD		37
#define GPIO_PUD_CLK_0	38
//mini uart register reference manual - page 8
#define BASE_ADD_AUX	(PBASE + 0x210000)
#define AUX_ENABLES		1			//Auxiliary enables
#define AUX_MU_IO_REG	16			//Mini Uart I/O Data
#define AUX_MU_IER_REG	17			//Mini Uart Interrupt Enable
#define AUX_MU_IIR_REG	18			//Mini Uart Interrupt Identify
#define AUX_MU_LCR_REG	19			//Mini Uart Line Control
#define AUX_MU_MCR_REG	20			//Mini Uart Modem Control
#define AUX_MU_LSR_REG	21			//Mini Uart Line Status
#define AUX_MU_MSR_REG	22			//Mini Uart Modem Status
#define AUX_MU_SCRATCH	23			//Mini Uart Scratch
#define AUX_MU_CNTL_REG	24			//Mini Uart Extra Control
#define AUX_MU_STAT_REG	25			//Mini Uart Extra Status
#define AUX_MU_BAUD_REG	26			//Mini Uart Baudrate
typedef enum{
	OUT = 0,
	IN = 1,
	ALT5 = 2,
}MODE_GPIO;


static DEFINE_MUTEX(uart_mutex);

static int major_number;
static char message[256]={0};
static unsigned char test[64]="SENDFROMRP: Hello PC! I am Raspberrypi";
static short size_of_message;
static int numberOpens=0;
static void __iomem *gpio_base;
static void __iomem *aux_base;
static struct class *uart_class=NULL;
static struct device* uart_device=NULL;

static int uart_open(struct inode *, struct file *);
static int uart_release(struct inode*, struct file*);
static ssize_t uart_read(struct file*, char *, size_t, loff_t *);
static ssize_t uart_write(struct file*, const char*, size_t, loff_t *);

static struct file_operations fops={
	.open = uart_open,
	.read = uart_read,
	.write = uart_write,
	.release = uart_release,
};
//gpio
static void SET_MODE_PIN(u32 pin, MODE_GPIO mode){
	u32 value;
	u32 *address = NULL;
	if(pin>40){
		printk(KERN_ALERT "Not found \n");
		return;
	}
	address = (u32 *)gpio_base + pin/10;
	value = ioread32(address);
	value &= ~(7<<((pin%10)*3));
	value |= (mode<<((pin%10)*3));
	iowrite32(value,address);
}
static void gpio_init(void){
	SET_MODE_PIN(14,ALT5);
	SET_MODE_PIN(15,ALT5);

	iowrite32(0, (u32 *)gpio_base + GPIO_PUD);
	//on datasheet delay 150 cycles (1 cycle = 1/250Mhz -> 0.6us -> delay 1us)
	udelay(1);
	iowrite32((1<<14)|(1<<15), (u32 *)gpio_base + GPIO_PUD);
	udelay(1);
	iowrite32(0, (u32 *)gpio_base + GPIO_PUD);
}
//uart function send a char arrays
static void uart_putc(unsigned char c){
	while(!((ioread32((u32 *)aux_base + AUX_MU_LSR_REG))&0x20));		//check transmit empty
	iowrite32(c,(u32 *)aux_base + AUX_MU_IO_REG);
}
static void sendstrings(unsigned char c[]){
	u8 i;
	for(i=0;i<(strlen(c));i++) uart_putc(c[i]);
	//gui ky tu xuong dong \r\n
	uart_putc(0x0D);
	uart_putc(0x0A);
}
static void mini_uart_init(void){
	iowrite32(1,(u32 *)aux_base + AUX_ENABLES);		//set enable mini uart
	iowrite32(0,(u32 *)aux_base + AUX_MU_IER_REG);
	iowrite32(0,(u32 *)aux_base + AUX_MU_CNTL_REG);	//disable receive va transmit miniuart, chuyen quyen RTS, tat CTS, cau hinh RTC CTS
	iowrite32(1,(u32 *)aux_base + AUX_MU_LCR_REG);	//set mode 8 bit
	iowrite32(0,(u32 *)aux_base + AUX_MU_MCR_REG);	//UART1_RTS line is high
	iowrite32(1, (u32 *)aux_base + AUX_MU_IIR_REG); //clear receive and transmit FIFO
	//set baudrate_reg = ( systemclock / 8 * baudrate ) - 1 = (250.000.000 / 8 * 115200) - 1 = 270
	iowrite32(270, (u32 *)aux_base + AUX_MU_BAUD_REG);
}

static int __init uart_init(void){
	printk(KERN_INFO "Init uart driver\n");
	gpio_base = ioremap(BASE_ADD_GPIO, 0x100);//0x100hex->256 byte -> 64 offset
	if(gpio_base == NULL){
		printk(KERN_ALERT "Can not mapping gpio mem phys to vitrual mem\n");
		return -ENOMEM;
	}
	printk(KERN_INFO "Map Gpio successfull\n");
	aux_base = ioremap(BASE_ADD_AUX, 0x100); //0x100hex->256 byte -> 64 offset
	if(aux_base == NULL){
		printk(KERN_ALERT "Can not mapping aux mem to vitrual mem\n");
		return -ENOMEM;
	}
	printk(KERN_INFO "Map Aux successfull\n");

	//init uart and gpio
	mini_uart_init();	
	gpio_init();
	iowrite32(3, (u32 *)aux_base + AUX_MU_CNTL_REG); //enable transmit and receive FIFO
	//register to device
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if(major_number < 0){
		printk(KERN_ALERT "Can not register to major_number: %d\n",major_number);
		return major_number;
	}
	printk(KERN_INFO "Register successfull with major_number: %d\n",major_number);

	uart_class = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(uart_class)){
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Can not create class \n");
		return PTR_ERR(uart_class);
	}
	printk(KERN_INFO "Create class successfull\n");

	uart_device = device_create(uart_class, NULL, MKDEV(major_number,0),NULL, DEVICE_NAME);
	if(IS_ERR(uart_device)){
		class_destroy(uart_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Can not create device driver\n");
		return PTR_ERR(uart_device);
	}
	printk(KERN_INFO "Create device driver correct\n");
	mutex_init(&uart_mutex);
	return 0;
}
static void __exit uart_exit(void){
	device_destroy(uart_class, MKDEV(major_number, 0));
	class_unregister(uart_class);
	class_destroy(uart_class);
	unregister_chrdev(major_number, DEVICE_NAME);
	mutex_destroy(&uart_mutex);
	printk(KERN_INFO "Module exiting ....\n");
}

static int uart_open(struct inode *inodep, struct file *filep){
	if(!mutex_trylock(&uart_mutex)){
		printk(KERN_INFO "Uart driver is using by another user\n");
		return -EBUSY;
	}
	numberOpens++;
	printk(KERN_INFO "User open %d times\n",numberOpens);
	return 0;
}
static int uart_release(struct inode* inodep, struct file *filep){
	printk(KERN_INFO "Uart driver is closing\n");
	mutex_unlock(&uart_mutex);
	return 0;
}
static ssize_t uart_read(struct file *filep, char *buf, size_t len, loff_t *offset){
	int error;
	error = copy_to_user(buf, message, size_of_message);
	if(error == 0){
		printk(KERN_INFO "Send to user successfull\n");
		return size_of_message;
	}
	else{
		printk(KERN_INFO "Fail with error: %d",error);
		return -EFAULT;
	}
}
static ssize_t uart_write(struct file *filep, const char *buf, size_t len, loff_t *offset){
	sprintf(message, "%s", buf);
	size_of_message = strlen(message);
	if(!strcmp(message,"SEND")){
		sendstrings(test);
	}
	printk(KERN_INFO "Receive message from user: %s\n",message);
	return len;
}

module_init(uart_init);
module_exit(uart_exit);