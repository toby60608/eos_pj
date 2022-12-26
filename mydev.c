
/***************************************************************************/
/**
 * \file led_driver.c
 * \details Simple GPIO driver explanation
 * \author EmbeTronicX
 * \Tested with Linux raspberrypi 5.4.51-v7l+
 *******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h> //copy_to/from_user()
#include <linux/gpio.h>    //GPIO
#include <linux/kstrtox.h>
#include <linux/interrupt.h>


dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp,
                        char __user *buf, size_t len, loff_t *off);
static ssize_t etx_write(struct file *filp,
                         const char *buf, size_t len, loff_t *off);
/******************************************************/

//#define USING_VIRTUAL_DEV_ONLY      1

/* /dev/etx_device input definition */
#define DEV_INPUT_NORMAL_LED_ON     51
#define DEV_INPUT_NORMAL_LED_OFF    52
#define DEV_INPUT_7SEG_OFF          53

/* virtual device */
unsigned long long virtual_gpio=0;
void mydev_gpio_set_value(unsigned gpio, int value)
{
#ifndef USING_VIRTUAL_DEV_ONLY
    gpio_set_value(gpio, value);
#endif
    printk("gpio %d --> %d\n",gpio,value);

    if (value==1)
        virtual_gpio |= (0x1 << gpio);
    if (value==0)
        virtual_gpio &= ~(0x1 << gpio);
}

#define LED_VALUE_INVALID           0xFF
int LED_value = LED_VALUE_INVALID;
void mydev_virtual_dev_set(int d)
{
    LED_value = d;
}
void mydev_virtual_dev_get(int *d)
{
    if(LED_value == LED_VALUE_INVALID)
    {
        *d = LED_VALUE_INVALID;
    }
    else
    {
        *d = LED_value;
        LED_value = LED_VALUE_INVALID;
    }
}

/* normal LED & interrupt */
#define NORMAL_LED_GPIO_PIN         22 //21
#define INTERRUPT_GPIO_PIN          12
unsigned int irq_number;
/* 7-eg LED */
/* map 7-seg pgfedcba tp GPIO pin */
#define LED_SEGMENT_NUM         8
typedef struct gpio_info
{
    char seg_name;
    int gpio_pin;
    bool requested;
}gpio_into_S;

/* camera will use GPIO 0 1 7 8 18 19 20 21 */


gpio_into_S gpioInfo[LED_SEGMENT_NUM] = {
    {.seg_name = 'p', .gpio_pin = 17, .requested = false}, //18
    {.seg_name = 'g', .gpio_pin = 13, .requested = false}, //23
    {.seg_name = 'f', .gpio_pin = 24, .requested = false},
    {.seg_name = 'e', .gpio_pin = 25, .requested = false},
    {.seg_name = 'd', .gpio_pin = 26, .requested = false}, //8
    {.seg_name = 'c', .gpio_pin = 27, .requested = false}, //7
    {.seg_name = 'b', .gpio_pin = 12, .requested = false},
    {.seg_name = 'a', .gpio_pin = 16, .requested = false},
};


typedef struct gpio_led_db
{
    int number;
    unsigned char pgfedcba;
} gpio_led_db_S;
gpio_led_db_S commonCathode_db[] =
{
        {.number = 0,  .pgfedcba = 0b00111111},
        {.number = 1,  .pgfedcba = 0b00000110},
        {.number = 2,  .pgfedcba = 0b01011011},
        {.number = 3,  .pgfedcba = 0b01001111},
        {.number = 4,  .pgfedcba = 0b01100110},
        {.number = 5,  .pgfedcba = 0b01101101},
        {.number = 6,  .pgfedcba = 0b01111101},
        {.number = 7,  .pgfedcba = 0b00000111},
        {.number = 8,  .pgfedcba = 0b01111111},
        {.number = 9,  .pgfedcba = 0b01101111},
        {.number = 10, .pgfedcba = 0b11110111}, //A
        {.number = 11, .pgfedcba = 0b11111100}, //B
        {.number = 12, .pgfedcba = 0b10111001}, //C
        {.number = 13, .pgfedcba = 0b11011110}, //D
};

#define LED_SEGMENT_NUM 8
int mydev_7segLED_write(int digit)
{
    int seg;

    for (seg=0; seg < LED_SEGMENT_NUM; seg++)
    {
        if( commonCathode_db[digit].pgfedcba & (0b10000000 >> seg) )
        {
            mydev_gpio_set_value(gpioInfo[seg].gpio_pin, 1);
        }
        else
        {
            mydev_gpio_set_value(gpioInfo[seg].gpio_pin, 0);
        }
    }
    printk("\n");

    mydev_virtual_dev_set(digit);
    return 0;
}
int mydev_7segLED_off(void)
{
    int seg;

    for (seg=0; seg < LED_SEGMENT_NUM; seg++)
    {
        mydev_gpio_set_value(gpioInfo[seg].gpio_pin, 0);
    }

    mydev_virtual_dev_set(DEV_INPUT_7SEG_OFF);

    return 0;
}



// File operation structure
static struct file_operations fops =
    {
        .owner = THIS_MODULE,
        .read = etx_read,
        .write = etx_write,
        .open = etx_open,
        .release = etx_release,
};
/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
    pr_info("Device File Opened...!!!\n");
    return 0;
}
/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
    pr_info("Device File Closed...!!!\n");
    return 0;
}
/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp,
                        char __user *buf, size_t len, loff_t *off)
{
    uint8_t gpio_state = 0;
    // reading GPIO value
    gpio_state = gpio_get_value(NORMAL_LED_GPIO_PIN);
    // write to user
    len = 1;
    if (copy_to_user(buf, &gpio_state, len) > 0)
    {
        pr_err("ERROR: Not all the bytes have been copied to user\n");
    }
    pr_info("Read function : NORMAL_LED_GPIO_PIN = %d \n", gpio_state);
    return 0;
}
/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp,
                         const char __user *buf, size_t len, loff_t *off)
{
    uint8_t rec_buf[10] = {0};
    int num;

    if (copy_from_user(rec_buf, buf, len) > 0)
    {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }
    //num = simple_strtol(rec_buf, NULL, 0);
    num=rec_buf[0];
    pr_info("Write Function : rec_buf = %s, num=%d\n", rec_buf,num);

    if(num == DEV_INPUT_NORMAL_LED_ON)
    {
        mydev_gpio_set_value(NORMAL_LED_GPIO_PIN, 1);
    }
    else if (num == DEV_INPUT_NORMAL_LED_OFF)
    {
        mydev_gpio_set_value(NORMAL_LED_GPIO_PIN, 0);
    }
    else if (num == DEV_INPUT_7SEG_OFF)
    {
        mydev_7segLED_off();
    }
    else if ((num>=0)&&(num<=13))
    {
        printk("Write %d to 7segLED\n", num);
        mydev_7segLED_write(num);
    }
    else
    {
        printk("skip the writing (num=%d)\n",num);
    }

    return len;
}

DECLARE_WAIT_QUEUE_HEAD(wait_queue_etx);
int wait_queue_flag = 0;

int wait_button(void *p)
{

    while(1) {
        pr_info("Waiting for button....\n");
        wait_event_interruptible(wait_queue_etx, wait_queue_flag != 0 );
        pr_info("button pressed\n");
        wait_queue_flag = 0;
    }
    do_exit(0);
    return 0;
}


#define GPIO_IN     1
#define GPIO_OUT    0
int mydev_gpio_init(int pin, int direction)
{
    char name[20]={0};

    if (gpio_is_valid(pin) == false)
    {
        pr_err("GPIO %d is not valid\n", pin);
        return -1;
    }
    sprintf(name, "GPIO_%d",pin);
    if (gpio_request(pin, name) < 0)
    {
        pr_err("ERROR: GPIO %d request\n", pin);
        return -1;
    }
    if(direction==GPIO_OUT)
        gpio_direction_output(pin, 0);
    if(direction==GPIO_IN)
        gpio_direction_input(pin);
    gpio_export(pin, false);
    return 0;
}


static irqreturn_t gpio_button_isr(int irq,void *dev_id)
{
//   static unsigned long flags = 0;
//   local_irq_save(flags);
    wait_queue_flag = 1;
    wake_up_interruptible(&wait_queue_etx);
//   local_irq_restore(flags);
  return IRQ_HANDLED;
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
    if ((dev_class = class_create(THIS_MODULE, "etx_class")) == NULL)
    {
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }
    /*Creating device*/
    if ((device_create(dev_class, NULL, dev, NULL, "etx_device")) == NULL)
    {
        pr_err("Cannot create the Device \n");
        goto r_device;
    }

    {
        int pin;
        for(pin=0;pin<LED_SEGMENT_NUM;pin++)
        {
            if (mydev_gpio_init(gpioInfo[pin].gpio_pin,GPIO_OUT) == -1)
                goto r_device;
        }
    }
    if( mydev_gpio_init(NORMAL_LED_GPIO_PIN,GPIO_OUT) == -1)
        goto r_device;

    /* GPIO interrupt init */
    if( mydev_gpio_init(INTERRUPT_GPIO_PIN,GPIO_IN) == -1)
        goto r_device;
#if 0
    if(gpio_set_debounce(INTERRUPT_GPIO_PIN, 200) < 0){
        pr_err("set %d debounce fail\n", INTERRUPT_GPIO_PIN);
    }
#endif
    irq_number = gpio_to_irq(INTERRUPT_GPIO_PIN);
    if (request_irq(irq_number,(void *)gpio_button_isr,IRQF_TRIGGER_RISING, "etx_device", NULL))
    {
        pr_err("register IRQ fail");
        goto r_device;
    }

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;

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
    gpio_unexport(NORMAL_LED_GPIO_PIN);
    gpio_free(NORMAL_LED_GPIO_PIN);
    gpio_unexport(INTERRUPT_GPIO_PIN);
    gpio_free(INTERRUPT_GPIO_PIN);
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Device Driver Remove...Done!!\n");
}
module_init(etx_driver_init);
module_exit(etx_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.32");