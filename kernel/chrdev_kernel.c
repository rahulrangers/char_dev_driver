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
#include <linux/mutex.h> // Include for mutex

#define MAX_BUF_SIZE 256
#define DEVICE_COUNT 2
#define IOCTL_RESET_BUFFER _IO('r', 1)

struct cdev_data {
    struct cdev cdev;
    unsigned char *user_data;
    struct mutex lock; // Mutex for concurrency control
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
            mutex_lock(&dev_data->lock); // Lock mutex before critical section
            memset(dev_data->user_data, 0, MAX_BUF_SIZE);
            mutex_unlock(&dev_data->lock); // Unlock mutex after critical section
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
    ssize_t ret = 0;

    printk(KERN_DEBUG "Entering: %s\n", __func__);
    mutex_lock(&dev_data->lock); // Lock mutex before accessing buffer

    udatalen = strlen(dev_data->user_data);
    if (count > udatalen)
        count = udatalen;

    if (copy_to_user(buf, dev_data->user_data, count) != 0) {
        printk(KERN_ERR "Copy data to user failed\n");
        ret = -EFAULT;
        goto out;
    }
    ret = count;

out:
    mutex_unlock(&dev_data->lock); // Unlock mutex after accessing buffer
    return ret;
}

static ssize_t cdev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    struct cdev_data *dev_data = file->private_data;
    size_t available_space, current_len, nbr_chars;
    ssize_t ret = 0;

    printk(KERN_DEBUG "Entering: %s\n", __func__);

    mutex_lock(&dev_data->lock); // Lock mutex before accessing buffer

    // Get the current length of the data in the buffer
    current_len = strlen(dev_data->user_data);
    available_space = MAX_BUF_SIZE - current_len;

    // Ensure there is enough space for the new data
    if (count > available_space) {
        printk(KERN_ERR "Not enough space in buffer for the new data\n");
        ret = -ENOMEM;  // Return an error if not enough space
        goto out;
    }

    // Append the new data to the existing data in the buffer
    nbr_chars = copy_from_user(dev_data->user_data + current_len, buf, count);
    if (nbr_chars == 0) {
        printk(KERN_DEBUG "Copied %zu bytes from the user\n", count);
        ret = count;
    } else {
        printk(KERN_ERR "Copy data from user failed\n");
        ret = -EFAULT;
    }

out:
    mutex_unlock(&dev_data->lock); // Unlock mutex after accessing buffer
    return ret;
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

        mutex_init(&mycdev_data[i].lock); // Initialize mutex for each device

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
        mutex_destroy(&mycdev_data[i].lock); // Destroy mutex
        kfree(mycdev_data[i].user_data);
    }
    class_unregister(mycdev_class);
    class_destroy(mycdev_class);
    unregister_chrdev_region(MKDEV(cdev_major, 0), DEVICE_COUNT);
}

MODULE_DESCRIPTION("A simple Linux char driver with concurrency control using mutex");
MODULE_VERSION("1.0");
MODULE_AUTHOR("Rahul Reddy <rahulreddypurmani123@gmail.com>");
MODULE_LICENSE("GPL");
