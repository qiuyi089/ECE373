#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/netdevice.h>

//Defined variable and function prototype.
#define E1000_CTRL 0x00000;
static int PCI_probe(struct pci_dev* pdev, const struct pci_device_id* ent);
static void pci_blinkDriver_remove(struct pci_dev* pdev);
#define DEVCNT 5
#define DEVNAME "LED_blink"

//myPCI struct for the LED driver
static struct myPCI_dev{
	struct cdev my_cdev;
        dev_t myPCI_node;
        int LED_val;
	struct pci_dev* pdev;
	void __iomem * hw_addr;
}myPCI;
//the address of the PCI device
static const struct pci_device_id pci_blinkDriver_address[] = {
	{PCI_DEVICE(0x8086,0x100e)},
	{},
};
//File operation for the PCI device
static struct pci_driver myPCI_blink_driver = {
	.name = DEVNAME,
	.id_table = pci_blinkDriver_address,
	.probe = PCI_probe,
	.remove = pci_blinkDriver_remove,
};

static int PCI_probe(struct pci_dev* pdev, const struct pci_device_id* ent){
	//unsigned long t_start, t_len;
	unsigned long bar_add;
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

	return 0;
}
static void pci_blinkDriver_remove(struct pci_dev * pdev){
	iounmap(myPCI.hw_addr);
	pci_release_selected_regions(pdev,pci_select_bars(pdev,IORESOURCE_MEM));
	printk(KERN_INFO "Blink Driver have been remove.\n");
}	

static int Blinky_open(struct inode *inode, struct file * file){
	printk(KERN_INFO"Blinky have been awoken.\n");
	return 0;
}

static ssize_t Blinky_read(struct file *file, char __user *buf,size_t len, loff_t *offset){
	char buffer[32];
	long int val = readl(myPCI.hw_addr + 0x00E00);
	if(*offset >= sizeof(int)){
		return 0;
	}	
	if(!buf){
		return -EINVAL;
	}
	snprintf(buffer,sizeof(int),"%lx",val);
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
        /* Make sure our user isn't bad... */
        if (!buf) {
                ret = -EINVAL;
                goto out;
        }
        printk(KERN_INFO "blinky_driver write function \n");
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
	//do another copy
/*
	if(copy_from_user(&val,buf,len)){
		ret = -EFAULT;
		goto mem_out;
	}	
	if(kern_buf[0] == 'E'){
		kern_buf = "14";
		val = 14;
	}
	else{
		kern_buf = "15";
		val = 15;
	}
	*/
        ret = kstrtol(kern_buf, 0, number);
        if(ret < 0){
                printk(KERN_INFO "This is not a valid number. \n");
                goto out;
        }
        else{
                myPCI.LED_val = *number;
                printk(KERN_INFO "Userspace wrote %d to us \n",myPCI.LED_val);
        }
     	writel(*number,myPCI.hw_addr + 0xE00);		
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
	printk(KERN_INFO "myPCI_init is open");
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

	return 0;

}

static void __exit myPCI_exit(void){
	pci_unregister_driver(&myPCI_blink_driver);
	cdev_del(&myPCI.my_cdev);
	unregister_chrdev_region(myPCI.myPCI_node, 1);
	printk(KERN_INFO "myPCI got deported");
} 


MODULE_AUTHOR("Martin Nguyen");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
module_init(myPCI_init);
module_exit(myPCI_exit);







