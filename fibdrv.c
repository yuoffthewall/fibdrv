#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "bn.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 100000

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

static long long (*fibs[3])(long long k);

static long long fib_sequence(long long k)
{
    long long f[] = {0, 1};

    for (int i = 2; i <= k; i++) {
        f[(i & 1)] += f[((i - 1) & 1)];
    }

    return f[(k & 1)];
}

static long long fast_doubling1(long long k)
{
    if (k == 0)
        return 0;
    else if (k <= 2)
        return 1;

    long long a = fast_doubling1(k / 2);
    long long b = fast_doubling1(k / 2 + 1);
    if (k & 1) {
        // k = (k - 1) / 2;
        return a * a + b * b;
    } else {
        // k = k / 2;
        return a * (2 * b - a);
    }
}

static long long fast_doubling2(long long n)
{
    int h = 64 - __builtin_clzll(n);
    long long a = 0;  // F(n) = 0
    long long b = 1;  // F(n + 1) = 1

    for (long long mask = 1 << (h - 1); mask; mask >>= 1) {
        // k = floor((n >> j) / 2)
        // then a = F(k), b = F(k + 1) now.
        long long c =
            a * (2 * b - a);  // F(2k) = F(k) * [ 2 * F(k + 1) – F(k) ]
        long long d = a * a + b * b;  // F(2k + 1) = F(k)^2 + F(k + 1)^2

        /*
         * n >> j is odd: n >> j = 2k + 1
         * F(n >> j) = F(2k + 1)
         * F(n >> j + 1) = F(2k + 2) = F(2k) + F(2k + 1)
         *
         * n >> j is even: n >> j = 2k
         * F(n >> j) = F(2k)
         * F(n >> j + 1) = F(2k + 1)
         */
        if (mask & n) {
            a = d;
            b = c + d;
        } else {
            a = c;
            b = d;
        }
    }
    return a;
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    bn *temp = fast_doubling_char(*offset);
    bn *encoded = bcd_encode(temp);
    unsigned long ret = copy_to_user(buf, encoded->num, encoded->len + 1);
    size_t len = encoded->len;
    freebn(temp);
    freebn(encoded);
    if (ret)
        return -1;
    return len;
    // return (ssize_t) fib_sequence(*offset);
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    if (size > 4)
        return 1;
    ktime_t kt;
    if (size == 3) {  // fib big
        kt = ktime_get();
        fib_linkedlist(*offset);
        kt = ktime_sub(ktime_get(), kt);
    } else if (size == 4) {  // fib big
        kt = ktime_get();
        fast_doubling_char(*offset);
        kt = ktime_sub(ktime_get(), kt);
    } else {
        kt = ktime_get();
        fibs[size](*offset);
        kt = ktime_sub(ktime_get(), kt);
    }

    return (ssize_t)(ktime_to_ns(kt));
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;
    fibs[0] = fib_sequence;
    fibs[1] = fast_doubling1;
    fibs[2] = fast_doubling2;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
