#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/timer.h>
#include <linux/timekeeping.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>



//Defined variable and function prototype.
#define E1000_CTRL 0x00000;
#define CTRL 0x00000;
#define CTRL_RST 0x00000;
#define RCTL 0x00100;
#define RDBAL 0x02800;
#define RDBAH 0x02804;
#define RDLEN 0x02808;
#define RDH 0x02810;
#define RDT 0x02818;
#define GCR 0x00400000;
#define GCR2 0x00000001;
//interrupt define
#define ICR 0x000C0;
#define ICS 0x000C8;
#define IMS 0x000D0;
#define IMC 0x000D8;
#define MDIC 0x00020;



static int PCI_probe(struct pci_dev* pdev, const struct pci_device_id* ent);
static void pci_blinkDriver_remove(struct pci_dev* pdev);
static void my_LEDblinker(struct timer_list * t_list);
static struct class * LED_blink;
#define DEVCNT 1
#define DEVNAME "LED_blink"
#define LED_ON 2047
#define LED_OFF 2046
//Receiver descriptor///////
struct led_drive_rx_desc{
	struct{
		__le64 buffer_addr;
		__le64 reserved;
	} read_store;
	struct{
		struct{
			__le32 mrq;
			union{
				__le32 rss;
				struct{
					__le16 ip_id;
					__le16 csum;
				}csum_ip;
			}hi_dword;

		}lower;
		struct{
			__le32 status_error;
			__le16 length;
			__le16 vlan;
		}upper;
	}writeBack;
};
struct buff_info{
		dma_addr_t phy;
		void * mem;
}buff_info[16];

//myPCI struct for the LED driver
static struct myPCI_dev{
	struct cdev my_cdev;
        dev_t myPCI_node;
        int LED_val;
	int blink_per_second;
	struct timer_list my_timer;
	struct pci_dev* pdev;
	void __iomem * hw_addr;
	bool LED_STATE;
	struct work_struct work;
	struct led_drive_rx_desc * ring;
}myPCI;
//the address of the PCI device
static const struct pci_device_id pci_blinkDriver_address[] = {
	{PCI_DEVICE(0x8086,0x100e)},
	{},
};

/* this shows up under /sys/modules/example5/parameters */
static int blink_rate = 2;
module_param(blink_rate, int, S_IRUSR | S_IWUSR);

//File operation for the PCI device
static struct pci_driver myPCI_blink_driver = {
	.name = DEVNAME,
	.id_table = pci_blinkDriver_address,
	.probe = PCI_probe,
	.remove = pci_blinkDriver_remove,
};
static void my_LEDblinker(struct timer_list * t_list){
	struct myPCI_dev * dev = from_timer(dev,t_list,my_timer);
	printk(KERN_INFO "my_LEDblinker is call\n");
	printk(KERN_INFO "blink per second is %d \n",myPCI.blink_per_second);
	if(myPCI.LED_STATE == true){
		myPCI.LED_STATE = false;
		writel(LED_OFF,myPCI.hw_addr + 0x00E00);
	}
	else{
		myPCI.LED_STATE = true;
		writel(LED_ON,myPCI.hw_addr + 0x00E00);
	}
	mod_timer(&dev -> my_timer, ((HZ/dev -> blink_per_second )+ jiffies));

}
static int PCI_probe(struct pci_dev* pdev, const struct pci_device_id* ent){
	unsigned long bar_add;
	struct workqueue_struct * led_work_queue;
	u32 rctl;
	int error;
	u64 rdba;
	printk(KERN_INFO "Probe got call.\n");

	bar_add = pci_select_bars(pdev,IORESOURCE_MEM);
	printk(KERN_INFO "Bar_add is %lx .\n",bar_add);

	pci_enable_device_mem(pdev);

	if(pci_request_selected_regions(pdev,bar_add,DEVNAME)){
		printk(KERN_ERR "Request selected region fail.\n");
		pci_release_selected_regions(pdev,pci_select_bars(pdev,IORESOURCE_MEM));
	}
	pci_set_master(pdev);

	if(!(myPCI.hw_addr = pci_ioremap_bar(pdev,0))){
		printk(KERN_ERR "Ioremap fail.\n");
		iounmap(myPCI.hw_addr);
		pci_release_selected_regions(pdev,pci_select_bars(pdev,IORESOURCE_MEM));
	}
	//initialize work queue
	INIT_WORK(&myPCI.work, led_work_queue);
	//disable interrupt
	WRITE_REG(&myPCI,IMC,0xffffffff);
	FLUSH(&myPCI);
	//global reset
	WRITE_REG(&myPCI,CTRL,(READ_REG(&myPCI,CTRL) | CTRL_RST));
	FLUSH(&myPCI);
	//wait for the PHY reset
	msleep(25);
	//disable intterrupt again after reset
	WRITE(&myPCI,IMC,0xffffffff);
	FLUSH(&myPCI);
	//reseserve the chicken bits
	WRITE_REG(&myPCI,GCR,(READ_REG(&myPCI,GCR) | (1<<22)));
	WRITE_REG(&myPCI,GCR2,(READ_REG(&myPCI,GCR2) | 1));
	FLUSH(&myPCI);
	//PHY setup
	WRITE_REG(&myPCI, MDIC, 0x1831af08);
	//allocate the rings, descriptors, and buffers, enable the Rx unit
	myPCmyPCI.ring = kzalloc(sizeof(struct ),GFP_KERNEL);
	if(!myPCI.ring){
		error = -ENOMEM;
		goto error_alloc_ring;
	}	
	
error_alloc_ring:

	return error;
}
static void pci_blinkDriver_remove(struct pci_dev * pdev){
	//stop the timer
	printk(KERN_INFO "delete timer \n");
	del_timer(&myPCI.my_timer);
	del_timer_sync(&myPCI.my_timer);


	iounmap(myPCI.hw_addr);
	pci_release_selected_regions(pdev,pci_select_bars(pdev,IORESOURCE_MEM));
	printk(KERN_INFO "Blink Driver have been remove.\n");
}	

static int Blinky_open(struct inode *inode, struct file * file){
	printk(KERN_INFO"Blinky have been awoken.\n");
	//Start the timer
	myPCI.blink_per_second = blink_rate;
	myPCI.LED_STATE = true;
	timer_setup(&myPCI.my_timer,my_LEDblinker,0);
	mod_timer(&myPCI.my_timer, ((HZ / myPCI.blink_per_second) + jiffies));
	return 0;
}

static ssize_t Blinky_read(struct file *file, char __user *buf,size_t len, loff_t *offset){
	char buffer[32];
	//long int val = readl(myPCI.hw_addr + 0x00E00); //read what is being store at that address
	printk(KERN_INFO "Blinky_read is call\n");
	if(*offset >= sizeof(int)){
		return 0;
	}	
	if(!buf){
		return -EINVAL;
	}
	snprintf(buffer,sizeof(int),"%d",myPCI.blink_per_second);
	if(copy_to_user(buf, buffer, strlen(buffer))){
		return -EINVAL;
	}
	*offset += len;
	return 0;
}
static ssize_t Blinky_write(struct file *file,const char __user *buf, size_t len, loff_t *offset){
	//int val;
	int ret;
	char * kern_buf;
	long * number;
	long number2;
	number = &number2;
        printk(KERN_INFO "Blinky_write is call\n");

        /* Make sure our user isn't bad... */
        if (!buf) {
                ret = -EINVAL;
                goto out;
        }
      	//check if it a negative number
	if(buf[0] == '-'){
		ret = EINVAL;
		goto out;
	}
        /* Get some memory to copy into... */
        kern_buf = kmalloc(len, GFP_KERNEL);

        /* ...and make sure it's good to go */
        if (!kern_buf) {
                ret = -ENOMEM;
                goto out;
        }
        /* Copy from the user-provided buffer */
        if (copy_from_user(kern_buf, buf, len)) {
                /* uh-oh... */
                ret = -EFAULT;
                goto mem_out;
        }
        ret = kstrtol(kern_buf, 0, number);
        if(ret < 0){
                printk(KERN_INFO "This is not a valid number. \n");
                goto out;
        }
        else{
                myPCI.LED_val = *number;
		myPCI.blink_per_second = *number;
                printk(KERN_INFO "Userspace wrote %d to us \n",myPCI.blink_per_second);
        }
     	//writel(*number,myPCI.hw_addr + 0xE00);  //write to LED0 to turn it ON/OFF
	timer_setup(&myPCI.my_timer,my_LEDblinker,0);
	mod_timer(&myPCI.my_timer, ((HZ / myPCI.blink_per_second) + jiffies));	
    	ret = len;
mem_out:
        kfree(kern_buf);
out:    
        return ret;
}

/* File operations for our device */
static struct file_operations mydev_fops = {
        .owner = THIS_MODULE,
        .open = Blinky_open,
        .read = Blinky_read,
        .write = Blinky_write,
};
//PCI init function.
static int __init myPCI_init(void){
	printk(KERN_INFO "myPCI_init is open\n");
	printk(KERN_INFO "PCI_driver module loading... blink_rate=%d\n", blink_rate);

	if (alloc_chrdev_region(&myPCI.myPCI_node, 0, DEVCNT, "myPCI_slave")) {
		printk(KERN_ERR "alloc_chrdev_region() failed!\n");
		return -1;
	}

	printk(KERN_INFO "Allocated %d devices at major: %d\n", DEVCNT,
			MAJOR(myPCI.myPCI_node));

        /* Initialize the character device and add it to the kernel */
        cdev_init(&myPCI.my_cdev, &mydev_fops);
        myPCI.my_cdev.owner = THIS_MODULE;

	if (cdev_add(&myPCI.my_cdev, myPCI.myPCI_node, DEVCNT)) {
                printk(KERN_ERR "cdev_add() failed!\n");
                /* clean up chrdev allocation */
                unregister_chrdev_region(myPCI.myPCI_node, DEVCNT);
                return -1;
        }
	//initialize the PCI driver
	if(pci_register_driver(&myPCI_blink_driver)){
		printk(KERN_ERR "PCI registration is fail because it is illegal.");
		pci_unregister_driver(&myPCI_blink_driver);
		unregister_chrdev_region(myPCI.myPCI_node,1);
		return -1;
	}
	printk(KERN_INFO "Class LED_blink is create in /dev/LED_blink \n");
	LED_blink = class_create(THIS_MODULE, DEVNAME);
	device_create(LED_blink, NULL, myPCI.myPCI_node,NULL,DEVNAME);
	return 0;

}

static void __exit myPCI_exit(void){
        printk(KERN_INFO "myPCI_exit is call time to die.\n");
	//destroy the dev entry and device class
	printk(KERN_INFO "delete /dev/LED_blink \n");
	device_destroy(LED_blink,myPCI.myPCI_node);
	class_destroy(LED_blink);
	//unbind the PCI driver
	printk(KERN_INFO "unregister PCI driver \n");
	pci_unregister_driver(&myPCI_blink_driver);
	//destroy cdev
	cdev_del(&myPCI.my_cdev);
	//clean up the devices
	unregister_chrdev_region(myPCI.myPCI_node, 1);
	printk(KERN_INFO "myPCI got deported");
} 


MODULE_AUTHOR("Martin Nguyen");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
module_init(myPCI_init);
module_exit(myPCI_exit);







