// #include <linux/module.h>
// #include <linux/fs.h>
// #include <linux/errno.h>
// #include <linux/miscdevice.h>
// #include <linux/kernel.h>
// #include <linux/major.h>
// #include <linux/mutex.h>
// #include <linux/proc_fs.h>
// #include <linux/seq_file.h>
// #include <linux/stat.h>
// #include <linux/init.h>
// #include <linux/device.h>
// #include <linux/tty.h>
// #include <linux/kmod.h>
// #include <linux/gfp.h>

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("ZYZ");
// MODULE_DESCRIPTION("A simple Linux driver");

// static int major_number;
// static char message[256] = {0};
// static short size_of_message;
// static struct class *simple_class;

// static int dev_open(struct inode *, struct file *);
// static int dev_release(struct inode *, struct file *);
// static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
// static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);


// static int dev_open(struct inode *inodep, struct file *filep) {
//     printk(KERN_INFO "SimpleDriver: Device has been opened\n");
//     return 0;
// }

// static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
//     int error_count = 0;

//     error_count = copy_to_user(buffer, message, size_of_message);
//     if (error_count == 0) {
//         printk(KERN_INFO "SimpleDriver: Sent %d characters to the user\n", size_of_message);
//         return (size_of_message = 0);
//     } else {
//         printk(KERN_INFO "SimpleDriver: Failed to send %d characters to the user\n", error_count);
//         return -EFAULT;
//     }
// }

// static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
//     sprintf(message, "%s(%zu letters)", buffer, len);
//     size_of_message = strlen(message);
//     printk(KERN_INFO "SimpleDriver: Received %zu characters from the user\n", len);
//     return len;
// }

// static int dev_release(struct inode *inodep, struct file *filep) {
//     printk(KERN_INFO "SimpleDriver: Device successfully closed\n");
//     return 0;
// }

// static struct file_operations fops = {
//     .open = dev_open,
//     .read = dev_read,
//     .write = dev_write,
//     .release = dev_release,
// };

// static int __init simple_driver_init(void) {
//     // int err;
//     printk(KERN_INFO "SimpleDriver: Initializing the SimpleDriver\n");

//     major_number = register_chrdev(0, "simple_driver", &fops);
//     simple_class = class_create(THIS_MODULE, "simple_class");

//     if (major_number < 0) {
//         printk(KERN_ALERT "SimpleDriver failed to register a major number\n");
//         return major_number;
//     }
//     device_create(simple_class, NULL, MKDEV(major_number, 0), NULL, "simple");
//     printk(KERN_INFO "SimpleDriver: registered correctly with major number %d\n", major_number);
//     return 0;
// }

// static void __exit simple_driver_exit(void) {
//     device_destroy(simple_class, MKDEV(major_number, 0));
//     class_destroy(simple_class);
//     unregister_chrdev(major_number, "simple_driver");
//     printk(KERN_INFO "SimpleDriver: Goodbye from the LKM!\n");
// }


// module_init(simple_driver_init);
// module_exit(simple_driver_exit);

/*
步骤 3: 编写Makefile
在同一个目录下创建一个 Makefile，以编译你的内核模块：

obj-m += simple_driver.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
步骤 4: 编译驱动
在终端中运行 make 命令来编译你的驱动。

步骤 5: 插入驱动
使用以下命令将驱动插入内核：

sudo insmod simple_driver.ko
步骤 6: 测试驱动
首先，检查设备的主设备号：

dmesg | grep SimpleDriver
然后，创建设备文件：

sudo mknod /dev/simple_driver c [major_number] 0
将 [major_number] 替换为你的驱动的主设备号。

现在，你可以使用 echo 和 cat 命令来测试写入和读取功能：

echo "simple, World!" > /dev/simple_driver
cat /dev/simple_driver
步骤 7: 移除驱动
当你完成测试后，可以使用以下命令来移除驱动：

sudo rmmod simple_driver
并删除设备文件：

sudo rm /dev/simple_driver
*/



#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>

static int major = 0;
static char kernel_buf[1024];
static struct class *simple_class;

#define MIN(a, b) (a < b ? a : b)

static ssize_t simple_drv_read(struct file *file, char __user *buf, size_t size, loff_t *offset) {
    int err;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = copy_to_user(buf, kernel_buf, MIN(size, 1024));
    return MIN(1024, size);
}

static ssize_t simple_drv_write(struct file *file, const char __user *buf, size_t size, loff_t *offset) {
    int err;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = copy_from_user(kernel_buf, buf, MIN(size, 1024));
    return MIN(1024, size);
}

static int simple_drv_open(struct inode *inode, struct file *file) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

static int simple_drv_close(struct inode *inode, struct file *file) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

static struct file_operations simple_drv = {
    .owner = THIS_MODULE,
    .open = simple_drv_open,
    .read = simple_drv_read,
    .write = simple_drv_write,
    .release = simple_drv_close,
};

static int __init simple_init(void) {
    int err;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    major = register_chrdev(0, "simple", &simple_drv);
    simple_class = class_create(THIS_MODULE, "simple_class");
    err = PTR_ERR(simple_class);
    if (IS_ERR(simple_class)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "simple");
        return -1;
    }
    device_create(simple_class, NULL, MKDEV(major, 0), NULL, "simple");
    return 0;
}

static void __exit simple_exit(void) {
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    device_destroy(simple_class, MKDEV(major, 0));
    class_destroy(simple_class);
    unregister_chrdev(major, "simple");
}

module_init(simple_init);
module_exit(simple_exit);
MODULE_LICENSE("GPL");