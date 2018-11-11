#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/fs.h>

MODULE_AUTHOR("PTH");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MODULE USING UART HAVE INTERRUPT");
MODULE_VERSION("1.0");
#define DEVICE_NAME				"UART_ZIGBEE"
#define CLASS_NAME				"Conmunicate_test"
static int major_number;
static struct device *uart_device;
static struct class *uart_class;
static int numberOpens=0;
static short size_of_message;
static char message[64];
static DEFINE_MUTEX(uart_mutex);

static void __iomem *uart_base;
#define PBASE					0x3f000000
#define BASE_ADDR_UART_0		(PBASE + 0x20100)
#define UART_DR					0						//Data register
#define UART_RSRECR				1						//Not comment
#define UART_FR					6						//Flag register
#define UART_IBRD				9						//Integer Baud rate divisor
#define UART_FBRD				10						//Fractional Baud rate divisor
#define UART_LCRH				11						//Line Control register
#define UART_CR					12						//Control register
#define UART_IFLS				13						//Interupt FIFO Level Select Register
#define UART_IMSC				14						//Interupt Mask Set Clear Register
#define UART_RIS				15						//Raw Interupt Status Register
#define UART_MIS				16						//Masked Interupt Status Register
#define UART_ICR				17						//Interupt Clear Register
#define UART_DMACR				18						//DMA Control Register
//Not use
#define UART_ITCR				32						//Test Control register
#define UART_ITIP				33						//Integration test input reg
#define UART_ITOP				34						//Integration test output reg
#define UART_TDR				35						//Test Data reg

static void uart_startup(void){
	u32 temp;
	//step 1: Disable interrupt
	iowrite32(0x0000, (u32 *)uart_base + UART_ICR);		//clear interrupt
	iowrite32(0x0000, (u32 *)uart_base + UART_IMSC);	//clear mask interrupt
	//step 1: Disable uart to config baudrate, ....
	temp = ioread32((u32 *)uart_base + UART_CR);
	temp &= ~(0x01);
	iowrite32(temp, (u32 *)uart_base + UART_CR);
	//wait to transmit and receive data current is done and uart not busy
	while(!(ioread32((u32 *)uart_base + UART_FR)&(1<<3)));	//check flag busy uart
	//disable FIFO to flush 
	temp = ioread32((u32 *)uart_base + UART_LCRH);
	temp &= ~(1<<4);
	iowrite32(temp, (u32 *)uart_base + UART_LCRH);
	/* Setting Line control register
	Stick parity : not use 			0
	WLEN: 8bit						1
									1
	FEN: Enable						1
	STP2: Disable					0
	FEN: Not use					0
	PEN: Disable					0
	BRK: Disable 					0
	*/
	iowrite32(0x70, (u32 *)uart_base + UART_LCRH);
	//set baudrate with uart clock: 250Mhz -> BaudrateDiv = 1627.604
	//using faction divitor and interger divitor: I: 0x65b , F: (0.604 x 64) + or - 0.5 = 39
	//Interger register: 0x65b, Faction divitor register: 0x27;
	iowrite32(0x65b, (u32 *)uart_base + UART_IBRD);
	iowrite32(0x27, (u32 *)uart_base + UART_FBRD);
	/*	Setting Control register not enable bit EN
		0 0 0 0 0 0 1 1 0 0 0 0 0 0 0 0
	*/
	iowrite32(0x300, (u32 *)uart_base + UART_CR);
	//Enable UART
	iowrite32(0x01, (u32 *)uart_base + UART_CR);
}
static void uart_send_char(char kytu[]){
	int i=0;
	//check transmit fifo is empty check bit 5 register FR
	while((ioread32((u32 *)uart_base + UART_FR)&(1<<5))!=0);
	//If empty: start send char
	while(i<8){
		printk(KERN_INFO "%c",kytu[i]);
		while((ioread32((u32 *)uart_base + UART_FR)&(1<<3))!=0);
		iowrite32(kytu[i], (u32 *)uart_base + UART_DR);
		i++;	
	};
}

static int uart_open(struct inode*, struct file*);
static int uart_release(struct inode*, struct file*);
static ssize_t uart_read(struct file*, char *, size_t, loff_t *);
static ssize_t uart_write(struct file*, const char *, size_t, loff_t *);

static struct file_operations fops={
	.open = uart_open,
	.read = uart_read,
	.write = uart_write,
	.release = uart_release,
};

static int __init uart_init(void){
	printk(KERN_INFO "Staring init _______\n");
	
	//test send 1 char to computer
	uart_send_char("Hello computer\n");
	uart_base = ioremap(BASE_ADDR_UART_0, 0x100);
	if(uart_base == NULL){
		printk(KERN_ALERT "Cannot map uart mem to vitrual mem\n");
		return -ENOMEM;
	}
	printk(KERN_INFO "Map uart successfull\n");


	//startup uart
	uart_startup();
	//test send 1 char to computer
	uart_send_char("Hello computer\n");


	//creat device uart with kernel
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if(major_number<0){
		printk(KERN_ALERT "Error with major number\n");
		return major_number;
	}

	uart_class = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(uart_class)){
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Create class failed\n");
		return PTR_ERR(uart_class);
	}

	uart_device = device_create(uart_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
	if(IS_ERR(uart_device)){
		class_destroy(uart_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Create device failed\n");
		return PTR_ERR(uart_device);
	}
	mutex_init(&uart_mutex);
	printk(KERN_INFO "Create device complete\n");
	return 0;
}
static void __exit uart_exit(void){
	mutex_destroy(&uart_mutex);
	device_destroy(uart_class, MKDEV(major_number, 0));
	class_unregister(uart_class);
	class_destroy(uart_class);
	unregister_chrdev(major_number, DEVICE_NAME);
	printk(KERN_INFO "Module is exiting\n");
}
static int uart_open(struct inode *inodep, struct file *filep){
	if(!mutex_trylock(&uart_mutex)){
		printk(KERN_ALERT "Cannot access uart device Because another process is using\n");
		return -EBUSY;
	}
	numberOpens++;
	printk(KERN_INFO "Open time : %d", numberOpens);
	return 0;
}
static int uart_release(struct inode *inodep, struct file *filep){
	mutex_unlock(&uart_mutex);
	printk(KERN_INFO "Module iss closing\n");
	return 0;
}
static ssize_t uart_read(struct file *filep, char *buf, size_t len, loff_t *offset){
	int error;
	error = copy_to_user(buf, message, size_of_message);
	if(!error){
		printk(KERN_INFO "Send %s to user successfull\n", message);
		return size_of_message;
	}
	else{
		printk(KERN_ALERT "Send to user failed\n");
		return error;
	}
} 
static ssize_t uart_write(struct file* filep, const char *buf, size_t len, loff_t *offset){
	sprintf(message, "%s", buf);
	size_of_message = strlen(message);
	printk(KERN_INFO "Receive from user: %s", message);
	return len;
}
module_init(uart_init);
module_exit(uart_exit);
