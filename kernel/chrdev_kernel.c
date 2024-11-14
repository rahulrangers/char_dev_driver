#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/ioctl.h>

#define MAX_BUF_SIZE 256
#define DEVICE_COUNT 2
#define IOCTL_RESET_BUFFER _IO('r', 1)

struct cdev_data {
    struct cdev cdev;
    unsigned char *user_data;
};

static int cdev_major = 0;
static struct class *mycdev_class = NULL;
static struct cdev_data mycdev_data[DEVICE_COUNT];

static int cdev_open(struct inode *inode, struct file *file) {
    struct cdev_data *dev_data;
    dev_data = container_of(inode->i_cdev, struct cdev_data, cdev);
    file->private_data = dev_data;
    printk(KERN_DEBUG "Entering: %s\n", __func__);
    return 0;
}

static int cdev_release(struct inode *inode, struct file *file) {
    printk(KERN_DEBUG "Entering: %s\n", __func__);
    return 0;
}

static long cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct cdev_data *dev_data = file->private_data;
    printk(KERN_DEBUG "Entering: %s\n", __func__);
    switch (cmd) {
        case IOCTL_RESET_BUFFER:
            memset(dev_data->user_data, 0, MAX_BUF_SIZE);
            printk(KERN_DEBUG "User data buffer reset\n");
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static ssize_t cdev_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    struct cdev_data *dev_data = file->private_data;
    size_t udatalen;
    printk(KERN_DEBUG "Entering: %s\n", __func__);
    udatalen = strlen(dev_data->user_data);
    if (count > udatalen)
        count = udatalen;
    if (copy_to_user(buf, dev_data->user_data, count) != 0) {
        printk(KERN_ERR "Copy data to user failed\n");
        return -EFAULT;
    }
    return count;
}

static ssize_t cdev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    struct cdev_data *dev_data = file->private_data;
    size_t udatalen = MAX_BUF_SIZE;
    size_t nbr_chars = 0;
    printk(KERN_DEBUG "Entering: %s\n", __func__);
    if (count < udatalen)
        udatalen = count;
    nbr_chars = copy_from_user(dev_data->user_data, buf, udatalen);
    if (nbr_chars == 0) {
        printk(KERN_DEBUG "Copied %zu bytes from the user\n", udatalen);
        printk(KERN_DEBUG "Received data from user: %s", dev_data->user_data);
    } else {
        printk(KERN_ERR "Copy data from user failed\n");
        return -EFAULT;
    }
    return count;
}

static const struct file_operations cdev_fops = {
    .owner = THIS_MODULE,
    .open = cdev_open,
    .release = cdev_release,
    .unlocked_ioctl = cdev_ioctl,
    .read = cdev_read,
    .write = cdev_write
};

int init_module(void) {
    int err;
    struct device *dev_ret;
    dev_t dev;
    int i;

    printk(KERN_DEBUG "Entering: %s\n", __func__);

    err = alloc_chrdev_region(&dev, 0, DEVICE_COUNT, "mycdev");
    if (err < 0) {
        printk(KERN_ERR "Allocate a range of char device numbers failed.\n");
        return err;
    }

    cdev_major = MAJOR(dev);
    printk(KERN_DEBUG "Device major number is: %d\n", cdev_major);

    mycdev_class = class_create(THIS_MODULE, "mycdev");
    if (IS_ERR(mycdev_class)) {
        unregister_chrdev_region(MKDEV(cdev_major, 0), DEVICE_COUNT);
        return PTR_ERR(mycdev_class);
    }

    for (i = 0; i < DEVICE_COUNT; i++) {
        mycdev_data[i].user_data = kzalloc(MAX_BUF_SIZE, GFP_KERNEL);
        if (!mycdev_data[i].user_data) {
            printk(KERN_ERR "Allocation memory for data buffer failed\n");
            err = -ENOMEM;
            goto cleanup;
        }

        cdev_init(&mycdev_data[i].cdev, &cdev_fops);
        mycdev_data[i].cdev.owner = THIS_MODULE;

        err = cdev_add(&mycdev_data[i].cdev, MKDEV(cdev_major, i), 1);
        if (err) {
            printk(KERN_ERR "Unable to add char device %d\n", i);
            goto cleanup;
        }

        dev_ret = device_create(mycdev_class, NULL, MKDEV(cdev_major, i), NULL, "mycdev-%d", i);
        if (IS_ERR(dev_ret)) {
            printk(KERN_ERR "Failed to create device mycdev-%d\n", i);
            cdev_del(&mycdev_data[i].cdev);
            err = PTR_ERR(dev_ret);
            goto cleanup;
        }
    }
    return 0;

cleanup:
    while (--i >= 0) {
        device_destroy(mycdev_class, MKDEV(cdev_major, i));
        cdev_del(&mycdev_data[i].cdev);
        kfree(mycdev_data[i].user_data);
    }
    class_destroy(mycdev_class);
    unregister_chrdev_region(MKDEV(cdev_major, 0), DEVICE_COUNT);
    return err;
}

void cleanup_module(void) {
    int i;
    printk(KERN_DEBUG "Entering: %s\n", __func__);
    for (i = 0; i < DEVICE_COUNT; i++) {
        device_destroy(mycdev_class, MKDEV(cdev_major, i));
        cdev_del(&mycdev_data[i].cdev);
        kfree(mycdev_data[i].user_data);
    }
    class_unregister(mycdev_class);
    class_destroy(mycdev_class);
    unregister_chrdev_region(MKDEV(cdev_major, 0), DEVICE_COUNT);
}

MODULE_DESCRIPTION("A simple Linux char driver with multiple devices");
MODULE_VERSION("1.0");
MODULE_AUTHOR("Rahul Reddy <rahulreddypurmani123@gmail.com>");
MODULE_LICENSE("GPL");
