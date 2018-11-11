#include <linux/init.h>            	// Chứa các hàm __init, __exit
#include <linux/module.h>           	// Các headers cho LKMs
#include <linux/kernel.h>           	// Chứa  types, macros, functions cho kernel 
MODULE_LICENSE("GPL");      //Khai báo license sử dụng
MODULE_AUTHOR("Phan Thanh Hung");      	//Khai báo tác giả
MODULE_DESCRIPTION("A simple Linux driver for the Raspberry Pi");  //Khai báo thông tin của module
MODULE_VERSION("0.1");		//Khai báo version của module

static char *name = "world";		//Tạo một biến static làm ví dụ argument truyền vào 
module_param(name, charp, S_IRUGO); ///< name,  charp = char ptr, S_IRUGO có thể đọc không thể đổi.
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log"); //Thông tin  parameter
 
static int __init helloRP_init(void){
printk(KERN_INFO "EBB: Hello %s from the RP LKM!\n", name);
return 0;
}
static void __exit helloRP_exit(void){
 printk(KERN_INFO "EBB: Goodbye %s from the RP LKM!\n", name);
}

module_init(helloRP_init);
module_exit(helloRP_exit);
