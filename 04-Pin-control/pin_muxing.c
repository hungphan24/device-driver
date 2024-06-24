#include <linux/module.h>           /* Defines functions such as module_init/module_exit */
#include <linux/gpio.h>             /* Defines functions such as gpio_request/gpio_free */
#include <linux/platform_device.h>  /* For platform devices */
#include <linux/gpio/consumer.h>    /* For GPIO Descriptor */
#include <linux/of.h>               /* For DT */  
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>

#define DRIVER_AUTHOR "hung.phan duchungeden@gmail.com"
#define DRIVER_DESC   "gpio subsystem"
#define PWM_MAX_DUTY_CYCLE 100

#define LOW     0
#define HIGH    1

#define DEVICE_NAME "gpio_pwm_switch"
#define CLASS_NAME "gpioswitch"

struct m_foo_dev {
    int size;
    dev_t dev_num;
    struct class *m_class;
    struct cdev m_cdev;
} mdev;

static int      m_open(struct inode *inode, struct file *file);
static int      m_release(struct inode *inode, struct file *file);
static ssize_t  m_read(struct file *filp, char __user *user_buf, size_t size,loff_t *offset);
static ssize_t  m_write(struct file *filp, const char *user_buf, size_t size, loff_t *offset);

static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .read       = m_read,
    .write      = m_write,
    .open       = m_open,
    .release    = m_release,
};

struct gpio_pwm_dev {
    struct gpio_desc *gpio1_18;
    struct pwm_device *pwm;
    struct pinctrl *pinctrl;
    struct pinctrl_state *pinctrl_gpio;
    struct pinctrl_state *pinctrl_pwm;
};
static struct gpio_pwm_dev gpio_pwm;

static const struct of_device_id gpiod_dt_ids[] = {
    { .compatible = "gpio-descriptor-based", },
    { /* sentinel */ }
};

static int my_pdrv_probe(struct platform_device *pdev) {
    struct device* dev = &pdev->dev;

    if (alloc_chrdev_region(&mdev.dev_num, 0, 1, DEVICE_NAME)) {
        pr_err("Failed to alloc chrdev region\n");
	    return -1;
    }

    pr_info("Major = %d Minor = %d\n", MAJOR(mdev.dev_num), MINOR(mdev.dev_num));

    cdev_init(&mdev.m_cdev, &fops);
    if ((cdev_add(&mdev.m_cdev, mdev.dev_num, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");
        goto rm_device_numb;
    }

    if ((mdev.m_class = class_create(THIS_MODULE, CLASS_NAME)) == NULL) {
        pr_err("Cannot create the struct class for my device\n");
        goto rm_device_numb;
    }

    if ((device_create(mdev.m_class, NULL, mdev.dev_num, NULL, DEVICE_NAME)) == NULL) {
        pr_err("Cannot create my device\n");
        goto rm_class;
    }

    gpio_pwm.gpio1_18 = gpiod_get(dev, "led30", GPIOD_OUT_LOW);
    if (IS_ERR(gpio_pwm.gpio1_18))
        return PTR_ERR(gpio_pwm.gpio1_18);

    gpio_pwm.pwm = devm_pwm_get(dev, NULL);
    if (IS_ERR(gpio_pwm.pwm))
        return PTR_ERR(gpio_pwm.pwm);

    gpio_pwm.pinctrl = devm_pinctrl_get(dev);
    if (IS_ERR(gpio_pwm.pinctrl))
        return PTR_ERR(gpio_pwm.pinctrl);

    gpio_pwm.pinctrl_gpio = pinctrl_lookup_state(gpio_pwm.pinctrl, "sleep");
    if (IS_ERR(gpio_pwm.pinctrl_gpio))
        return PTR_ERR(gpio_pwm.pinctrl_gpio);

    gpio_pwm.pinctrl_pwm = pinctrl_lookup_state(gpio_pwm.pinctrl, "default");
    if (IS_ERR(gpio_pwm.pinctrl_pwm))
        return PTR_ERR(gpio_pwm.pinctrl_pwm);


    printk(KERN_INFO "HUNGPHAN Hello world\n");
    return 0;

rm_device:
    device_destroy(mdev.m_class, mdev.dev_num);
rm_class:
    class_destroy(mdev.m_class);
rm_device_numb:
    unregister_chrdev_region(mdev.dev_num, 1);
    return -1;
}

static int my_pdrv_remove(struct platform_device *pdev) {
    device_destroy(mdev.m_class, mdev.dev_num);
    class_destroy(mdev.m_class);
    cdev_del(&mdev.m_cdev);
    unregister_chrdev_region(mdev.dev_num, 3);
    pwm_disable(gpio_pwm.pwm);
    printk(KERN_INFO "hungphan exit\n");
    return 0;
}

/* platform driver */
static struct platform_driver mypdrv = {
    .probe = my_pdrv_probe,
    .remove = my_pdrv_remove,
    .driver = {
        .name = "descriptor-based",
        .of_match_table = of_match_ptr(gpiod_dt_ids),
        .owner = THIS_MODULE,
    },
};
module_platform_driver(mypdrv);

static int m_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device has been opened\n");
    return 0;
}

static int m_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t m_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[16];
    long mode, value;
    int ret;
    unsigned long period;
    unsigned long duty_cycle;

    if (count >= sizeof(kbuf))
        return -EINVAL;

    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;

    kbuf[count] = '\0';

    ret = sscanf(kbuf, "%ld %ld", &mode, &value);
    if (ret != 2)
        return -EINVAL;

    if (mode == 1) {
        // Disable PWM
        pwm_disable(gpio_pwm.pwm);
        
        // Select GPIO state
        pinctrl_select_state(gpio_pwm.pinctrl, gpio_pwm.pinctrl_gpio);
        if (value == 1) {
            gpiod_set_value(gpio_pwm.gpio1_18, 1);
        } else if (value == 0) {
            gpiod_set_value(gpio_pwm.gpio1_18, 0);
        } else {
            return -EINVAL;
        }
    } else if (mode == 0) {
        // Set GPIO to low (disable GPIO output)
        gpiod_set_value(gpio_pwm.gpio1_18, 0);
        if (!gpio_pwm.pwm) {
            printk(KERN_INFO "PWM is not exported\n");
        }

        // Select PWM state
        pinctrl_select_state(gpio_pwm.pinctrl, gpio_pwm.pinctrl_pwm);

        // Get PWM period
        period = 5000000;
        printk(KERN_INFO "PWM period: %lu\n", period);

        if (value >= 0 && value <= PWM_MAX_DUTY_CYCLE) {
            duty_cycle = value * period / PWM_MAX_DUTY_CYCLE;
            printk(KERN_INFO "Duty_cycle = %lu\n", duty_cycle);
            pwm_config(gpio_pwm.pwm, duty_cycle, period);
            pwm_enable(gpio_pwm.pwm);
        } else {
            return -EINVAL;
        }
    } else {
        return -EINVAL;
    }

    return count;
}

static ssize_t  m_read(struct file *filp, char __user *user_buf, size_t size,loff_t *offset) {
    return size;
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION("1.0");