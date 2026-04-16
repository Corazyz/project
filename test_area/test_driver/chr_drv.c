#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/mm_types.h>
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/cdev.h>


#define CHRDEVBASE_MAJOR 511
#define CHRDEVBASE_NAME "chrdevbase"
static uint irqLine = 0;
module_param(irqLine, uint, 0644);

static char readbuf[100];
static char writebuf[100];
static char kerneldata[] = {"kernel data!"};

// hardware register
#define GPIO_TEST_BASE (0x1234567)
static void __iomem *GPIO_TEST;

static int chrdevbase_open(struct inode *inode, struct file *filp) {
    unsigned int minor = iminor(inode);
    printk(KERN_INFO "Opening device minor: %u\n", minor);
    return 0;
}

// read data from device
static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt) {
    int ret = 0;
    // unsigned char databuf[1];
// read hardware register
#if 0
    // read register status
    databuf[0] = readl(GPIO_TEST);
    ret = copy_to_user(buf, databuf, cnt);
// read kernel mem
#else
    //copy test data to read data buffer
    memcpy(readbuf, kerneldata, sizeof(kerneldata));
    // kernel data(read buffer) copy to user space
    ret = copy_to_user(buf, readbuf, cnt);
#endif

    if (ret == 0) printk("kernel senddata ok!\n");
    else printk("kernel senddata failed!\n");
    return 0;
}

static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt) {
    int ret = 0;

// write hardware register
#if 0
    writel(buf[0], GPIO_TEST);
// write kernel mem
#else
    // user data copy to kernel space(write buffer)
    ret = copy_from_user(writebuf, buf, cnt);
#endif
    if (ret == 0) printk("kernel receivedata: %s\n", writebuf);
    else printk("kernel receivedata failed!\n");
    return 0;
}

// close/release device
static int chrdevbase_release(struct inode *inode, struct file *filp) {
    return 0;
}

// device operation function
static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
    .release = chrdevbase_release,
};

/*drive init func*/
static struct class *chrdevbase_class;
static struct device *chrdevbase_device;
static dev_t devno;

static int __init chrdevbase_init(void)
{
    int ret;
    ret = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
    printk(KERN_INFO "Module loaded with IRQ line: %u\n", irqLine);
    printk("chrdevbase_init() major=%d\n", MAJOR(devno));
    return 0;
}

/*driver exit function*/

static void __exit chrdevbase_exit(void)
{
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
    printk("chrdevbase_exit()\n");
}

/*designate two functions above as the init and exit functions of the driver*/
module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPL");
