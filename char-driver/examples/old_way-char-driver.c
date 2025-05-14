#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME     "MY_CHAR_Device"
static int num_major;

static int MY_CHAR_open(struct inode *inode, struct file *file)
{
        printk(KERN_INFO "MY_CHAR_open called\n");
        return 0;
}

static int MY_CHAR_release(struct inode *inode, struct file *file)
{
        printk(KERN_INFO "MY_CHAR_release called\n");
        return 0;
}

static ssize_t MY_CHAR_read(struct file *file,  
                           char *buf,  
                           size_t len,  
                           loff_t * off)
{
        printk(KERN_INFO "MY_CHAR_read called\n");     
        return 0;
}

static ssize_t MY_CHAR_write(struct file *file,
                            const char *buf,
                            size_t len,
                            loff_t * off)
{

        printk(KERN_INFO "MY_CHAR_write called\n"); 
        return len;
}



static struct file_operations fops= {
        .open = MY_CHAR_open,
        .release = MY_CHAR_release,
        .read = MY_CHAR_read,
        .write = MY_CHAR_write,
};


static int __init MY_CHAR_init(void)
{

        printk(KERN_INFO "Entering Test Character Driver \n");

        num_major=register_chrdev(0, DEVICE_NAME, &fops);
        printk(KERN_INFO "Major Number = %d \n",num_major);
        printk(KERN_INFO "Name =  %s \n",DEVICE_NAME);
        printk(KERN_INFO "Generate the device file with\
                mknod /dev/%s c %d 0 \n",DEVICE_NAME,num_major);
	/*
	 *After creating device try write some data
	 *sudo sh -c 'echo 2 > /dev/device_name'
	 *Now check the dmesg, open, write and release function will be called
	 * */
        return 0 ;

}

static void __exit MY_CHAR_cleanup(void)
{
        unregister_chrdev(num_major, DEVICE_NAME);
        printk(KERN_INFO "Exiting Test Character Driver \n");
}

module_init(MY_CHAR_init);
module_exit(MY_CHAR_cleanup);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bharath J");                       
MODULE_DESCRIPTION("Character Device Driver");            
MODULE_SUPPORTED_DEVICE("MY_CHARchar");

