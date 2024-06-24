#include <linux/module.h>   /* This module defines functions such as module_init/module_exit */
#include <linux/io.h>       /* This module defines functions such as ioremap/iounmap */
#include "gpio.h"           /* Gpio modules */

#define DRIVER_AUTHOR "phonglt15 p.linuxfromscratch@gmail.com"
#define DRIVER_DESC   "Turn on and off led"

uint32_t __iomem *gpio0_base_addr;

/* Constructor */
static int __init gpio_init(void)
{
    printk(KERN_INFO "Initializing GPIO module...\n");
    gpio0_base_addr = ioremap(GPIO0_ADDR_BASE, GPIO0_ADDR_SIZE);
    if(!gpio0_base_addr) {
        printk(KERN_ERR "Failed to ioremap GPIO0 base address\n");
        return -ENOMEM;
    }
        
    
    /* Set gpio0_30 as output mode and turn on */
    printk(KERN_INFO "Setting GPIO0_30 as output mode...\n");
    *(gpio0_base_addr + GPIO_OE_OFFSET / 4) &= ~GPIO0_30;
    printk(KERN_INFO "Setting GPIO0_30 as output mode...\n");
    *(gpio0_base_addr + GPIO_SETDATAOUT_OFFSET / 4) |= GPIO0_30;

    pr_info("Hello! Initizliaze successfully!\n");
    return 0;
}

/* Destructor */
static void __exit gpio_exit(void)
{
    *(gpio0_base_addr + GPIO_CLEARDATAOUT_OFFSET / 4) |= GPIO0_30;
    iounmap(gpio0_base_addr);

    pr_info("Good bye my fen !!!\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC); 
MODULE_VERSION("1.0"); 