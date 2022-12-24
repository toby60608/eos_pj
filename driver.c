#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <string.h>

// BUTTON is connected to this GPIO
#define GPIO_16 16

// BUZZER is connected to this GPIO
#define GPIO_20 20

// LED is connected to this GPIO
#define GPIO_21 21

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);

/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t etx_write(struct file *filp, const char *buf, size_t len, loff_t *off);
/******************************************************/

// File operation structure
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = etx_read,
    .write = etx_write,
    .open = etx_open,
    .release = etx_release,
};

/*
 This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
    pr_info("Device File Opened...!!!\n");
    return 0;
}

/*
 This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
    pr_info("Device File Closed...!!!\n");
    return 0;
}

/*
 This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    uint8_t gpio_state = 0;

    char *component;
    strcpy(component, buf);
    component = strtok(component, "_");

    if (strstr(component, "btn") != NULL)
    {

        gpio_state = gpio_get_value(GPIO_16);
        len = 1;
        if (copy_to_user(buf, &gpio_state, len) > 0)
        {
            pr_err("ERROR: BUTTON copied to user fail !!!\n");
        }
        pr_info("Read BUTTON : GPIO_16 = %d \n", gpio_state);
    }

    return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    // uint8_t rec_buf[10] = {0};

    char rec_buf[50];

    if (copy_from_user(rec_buf, buf, len) > 0)
    {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }

    char *component;
    strcpy(component, buf);
    component = strtok(component, "_");

    char *act;

    if (strstr(component, "lcd") != NULL)
    {
    }
    else if (strstr(component, "led") != NULL)
    {
        act = strtok(NULL, "_");
        pr_info("Write Function : GPIO_21 Set = %s\n", act);
        if (strcmp(act, "1") == 0)
        {
            gpio_set_value(GPIO_21, 1);
        }
        else if (strcmp(act, "0") == 0)
        {
            gpio_set_value(GPIO_21, 0);
        }
        else
        {
            pr_err("Unknown command : Please provide eit or 0 \n");
            return len;
        }
    }
    else if (strstr(cpmponent, "buz") != NULL)
    {
        act = strtok(NULL, "_");
        pr_info("Write Function : GPIO_20 Set = %s\n", act);
        if (strcmp(act, "1") == 0)
        {
            gpio_set_value(GPIO_20, 1);
        }
        else if (strcmp(act, "0") == 0)
        {
            gpio_set_value(GPIO_20, 0);
        }
        else
        {
            pr_err("Unknown command : Please provide either 1 or 0 \n");
            return len;
        }
    }
    else if (strstr(component, "btn") != NULL)
    {
        act = strtok(NULL, "_");
        pr_info("Write Function : GPIO_16 Set = %s\n", act);
        if (strcmp(act, "1") == 0)
        {
            gpio_set_value(GPIO_16, 1);
        }
        else if (strcmp(act, "0") == 0)
        {
            gpio_set_value(GPIO_16, 0);
        }
        else
        {
            pr_err("Unknown command : Please provide either 1 or 0 \n");
            return len;
        }
    }
    else if (strstr(component, "cam") != NULL)
    {
    }

    return len;
}

/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
    /*Allocating Major number*/
    if ((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0)
    {
        pr_err("Cannot allocate major number\n");
        goto r_unreg;
    }

    pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

    /*Creating cdev structure*/
    cdev_init(&etx_cdev, &fops);

    /*Adding character device to the system*/
    if ((cdev_add(&etx_cdev, dev, 1)) < 0)
    {
        pr_err("Cannot add the device to the system\n");
        goto r_del;
    }

    /*Creating struct class*/
    if ((dev_class = class_create(THIS_MODULE, "eos7_class")) == NULL)
    {
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }

    /*Creating device*/
    if ((device_create(dev_class, NULL, dev, NULL, "eos7_driver")) == NULL)
    {
        pr_err("Cannot create the Device \n");
        goto r_device;
    }

    // Requesting the GPIO
    if (gpio_request(GPIO_16, "GPIO_16") < 0)
    {
        pr_err("ERROR: GPIO %d request\n", GPIO_16);
        goto r_gpio;
    }
    // Requesting the GPIO
    if (gpio_request(GPIO_20, "GPIO_20") < 0)
    {
        pr_err("ERROR: GPIO %d request\n", GPIO_20);
        goto r_gpio;
    }
    // Requesting the GPIO
    if (gpio_request(GPIO_21, "GPIO_21") < 0)
    {
        pr_err("ERROR: GPIO %d request\n", GPIO_21);
        goto r_gpio;
    }

    // configure the GPIO as output
    gpio_direction_output(GPIO_16, 0);
    gpio_direction_output(GPIO_20, 0);
    gpio_direction_output(GPIO_21, 0);

    /* Using this call the GPIO 21 will be visible in /sys/class/gpio/
    ** Now you can change the gpio values by using below commands also.
    ** echo 1 > /sys/class/gpio/gpio21/value (turn ON the LED) ** echo 0 > /sys/class/gpio/gpio21/value (turn OFF the LED)
    ** cat /sys/class/gpio/gpio21/value (read the value LED) **
    ** the second argument prevents the direction from being changed. */
    gpio_export(GPIO_16, false);
    gpio_export(GPIO_20, false);
    gpio_export(GPIO_21, false);

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;

r_gpio:
    gpio_free(GPIO_16);
    gpio_free(GPIO_20);
    gpio_free(GPIO_21);
r_device:
    device_destroy(dev_class, dev);
r_class:
    class_destroy(dev_class);
r_del:
    cdev_del(&etx_cdev);
r_unreg:
    unregister_chrdev_region(dev, 1);

    return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{

    gpio_unexport(GPIO_16);
    gpio_free(GPIO_16);
    gpio_unexport(GPIO_21);
    gpio_free(GPIO_20);
    gpio_unexport(GPIO_21);
    gpio_free(GPIO_21);

    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Device Driver Remove...Done!!\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xingfu");
MODULE_DESCRIPTION("EOS7 device driver - GPIO Driver");
MODULE_VERSION("1.00");
