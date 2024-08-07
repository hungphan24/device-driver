#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define SSD1306_MAX_SEG 128
#define SSD1306_MAX_LINE 7
#define SSD1306_DEF_FONT_SIZE 5

// echo string > /dev/ssd1306


// Define GPIO pins
#define DC_PIN 48
#define RST_PIN 49

#define DEVICE_NAME "ssd1306"
#define CLASS_NAME "spi_ssd1306"

struct m_spi_dev {
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

struct ssd1306_spi_module {
	struct spi_device *device;
	uint8_t line_num;
	uint8_t cursor_position;
	uint8_t font_size;
	struct cdev dev;
};

struct ssd1306_spi_module *module1;

static const unsigned char ssd1306_font[][SSD1306_DEF_FONT_SIZE] = {
		{0x00, 0x00, 0x00, 0x00, 0x00}, // space
		{0x00, 0x00, 0x2f, 0x00, 0x00}, // !
		{0x00, 0x07, 0x00, 0x07, 0x00}, // "
		{0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
		{0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
		{0x23, 0x13, 0x08, 0x64, 0x62}, // %
		{0x36, 0x49, 0x55, 0x22, 0x50}, // &
		{0x00, 0x05, 0x03, 0x00, 0x00}, // '
		{0x00, 0x1c, 0x22, 0x41, 0x00}, // (
		{0x00, 0x41, 0x22, 0x1c, 0x00}, // )
		{0x14, 0x08, 0x3E, 0x08, 0x14}, // *
		{0x08, 0x08, 0x3E, 0x08, 0x08}, // +
		{0x00, 0x00, 0xA0, 0x60, 0x00}, // ,
		{0x08, 0x08, 0x08, 0x08, 0x08}, // -
		{0x00, 0x60, 0x60, 0x00, 0x00}, // .
		{0x20, 0x10, 0x08, 0x04, 0x02}, // /
		{0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
		{0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
		{0x42, 0x61, 0x51, 0x49, 0x46}, // 2
		{0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
		{0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
		{0x27, 0x45, 0x45, 0x45, 0x39}, // 5
		{0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
		{0x01, 0x71, 0x09, 0x05, 0x03}, // 7
		{0x36, 0x49, 0x49, 0x49, 0x36}, // 8
		{0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
		{0x00, 0x36, 0x36, 0x00, 0x00}, // :
		{0x00, 0x56, 0x36, 0x00, 0x00}, // ;
		{0x08, 0x14, 0x22, 0x41, 0x00}, // <
		{0x14, 0x14, 0x14, 0x14, 0x14}, // =
		{0x00, 0x41, 0x22, 0x14, 0x08}, // >
		{0x02, 0x01, 0x51, 0x09, 0x06}, // ?
		{0x32, 0x49, 0x59, 0x51, 0x3E}, // @
		{0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
		{0x7F, 0x49, 0x49, 0x49, 0x36}, // B
		{0x3E, 0x41, 0x41, 0x41, 0x22}, // C
		{0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
		{0x7F, 0x49, 0x49, 0x49, 0x41}, // E
		{0x7F, 0x09, 0x09, 0x09, 0x01}, // F
		{0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
		{0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
		{0x00, 0x41, 0x7F, 0x41, 0x00}, // I
		{0x20, 0x40, 0x41, 0x3F, 0x01}, // J
		{0x7F, 0x08, 0x14, 0x22, 0x41}, // K
		{0x7F, 0x40, 0x40, 0x40, 0x40}, // L
		{0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
		{0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
		{0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
		{0x7F, 0x09, 0x09, 0x09, 0x06}, // P
		{0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
		{0x7F, 0x09, 0x19, 0x29, 0x46}, // R
		{0x46, 0x49, 0x49, 0x49, 0x31}, // S
		{0x01, 0x01, 0x7F, 0x01, 0x01}, // T
		{0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
		{0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
		{0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
		{0x63, 0x14, 0x08, 0x14, 0x63}, // X
		{0x07, 0x08, 0x70, 0x08, 0x07}, // Y
		{0x61, 0x51, 0x49, 0x45, 0x43}, // Z
		{0x00, 0x7F, 0x41, 0x41, 0x00}, // [
		{0x55, 0xAA, 0x55, 0xAA, 0x55}, // Backslash (Checker pattern)
		{0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
		{0x04, 0x02, 0x01, 0x02, 0x04}, // ^
		{0x40, 0x40, 0x40, 0x40, 0x40}, // _
		{0x00, 0x03, 0x05, 0x00, 0x00}, // `
		{0x20, 0x54, 0x54, 0x54, 0x78}, // a
		{0x7F, 0x48, 0x44, 0x44, 0x38}, // b
		{0x38, 0x44, 0x44, 0x44, 0x20}, // c
		{0x38, 0x44, 0x44, 0x48, 0x7F}, // d
		{0x38, 0x54, 0x54, 0x54, 0x18}, // e
		{0x08, 0x7E, 0x09, 0x01, 0x02}, // f
		{0x18, 0xA4, 0xA4, 0xA4, 0x7C}, // g
		{0x7F, 0x08, 0x04, 0x04, 0x78}, // h
		{0x00, 0x44, 0x7D, 0x40, 0x00}, // i
		{0x40, 0x80, 0x84, 0x7D, 0x00}, // j
		{0x7F, 0x10, 0x28, 0x44, 0x00}, // k
		{0x00, 0x41, 0x7F, 0x40, 0x00}, // l
		{0x7C, 0x04, 0x18, 0x04, 0x78}, // m
		{0x7C, 0x08, 0x04, 0x04, 0x78}, // n
		{0x38, 0x44, 0x44, 0x44, 0x38}, // o
		{0xFC, 0x24, 0x24, 0x24, 0x18}, // p
		{0x18, 0x24, 0x24, 0x18, 0xFC}, // q
		{0x7C, 0x08, 0x04, 0x04, 0x08}, // r
		{0x48, 0x54, 0x54, 0x54, 0x20}, // s
		{0x04, 0x3F, 0x44, 0x40, 0x20}, // t
		{0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
		{0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
		{0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
		{0x44, 0x28, 0x10, 0x28, 0x44}, // x
		{0x1C, 0xA0, 0xA0, 0xA0, 0x7C}, // y
		{0x44, 0x64, 0x54, 0x4C, 0x44}, // z
		{0x00, 0x10, 0x7C, 0x82, 0x00}, // {
		{0x00, 0x00, 0xFF, 0x00, 0x00}, // |
		{0x00, 0x82, 0x7C, 0x10, 0x00}, // }
		{0x00, 0x06, 0x09, 0x09, 0x06}	// ~ (Degrees)
};


static int ssd1306_send_command(struct spi_device *spi, const u8 *cmd, size_t len)
{
    gpio_set_value(DC_PIN, 0); // Command mode
    return spi_write(spi, cmd, len);
}

static int ssd1306_send_data(struct spi_device *spi, const u8 *data, size_t len)
{
    gpio_set_value(DC_PIN, 1); // Data mode
    return spi_write(spi, data, len);
}

static void ssd1306_write(struct ssd1306_spi_module *module, bool is_cmd, unsigned char data)
{
    unsigned char buf[1] = {data};

    if (is_cmd) {
        ssd1306_send_command(module->device, buf, 1);
    } else {
        ssd1306_send_data(module->device, buf, 1);
    }
}

static void ssd1306_set_cursor(struct ssd1306_spi_module *module, uint8_t line_num, uint8_t cursor_position)
{
	if ((line_num <= SSD1306_MAX_LINE) && (cursor_position < SSD1306_MAX_SEG)) {
		module->line_num = line_num;			   // Save the specified line number
		module->cursor_position = cursor_position; // Save the specified cursor position
		ssd1306_write(module, true, 0x21);				   // cmd for the column start and end address
		ssd1306_write(module, true, cursor_position);	   // column start addr
		ssd1306_write(module, true, SSD1306_MAX_SEG - 1);  // column end addr
		ssd1306_write(module, true, 0x22);				   // cmd for the page start and end address
		ssd1306_write(module, true, line_num);			   // page start addr
		ssd1306_write(module, true, SSD1306_MAX_LINE);	   // page end addr
	}
}

static void ssd1306_clear(struct ssd1306_spi_module *module)
{
	unsigned int total = 128 * 8; 
	int i;

	for (i = 0; i < total; i++) {
		ssd1306_write(module, false, 0);
	}
}


static int ssd1306_init_display(struct ssd1306_spi_module *module)
{
	gpio_set_value(RST_PIN, 0);
	udelay(2);
	gpio_set_value(RST_PIN, 1);
    msleep(100);
	ssd1306_write(module, true, 0xAE); // Entire Display OFF
	ssd1306_write(module, true, 0xD5); // Set Display Clock Divide Ratio and Oscillator Frequency
	ssd1306_write(module, true, 0x80); // Default Setting for Display Clock Divide Ratio and Oscillator Frequency that is recommended
	ssd1306_write(module, true, 0xA8); // Set Multiplex Ratio
	ssd1306_write(module, true, 0x3F); // 64 COM lines
	ssd1306_write(module, true, 0xD3); // Set display offset
	ssd1306_write(module, true, 0x00); // 0 offset
	ssd1306_write(module, true, 0x40); // Set first line as the start line of the display
	ssd1306_write(module, true, 0x8D); // Charge pump
	ssd1306_write(module, true, 0x14); // Enable charge dump during display on
	ssd1306_write(module, true, 0x20); // Set memory addressing mode
	ssd1306_write(module, true, 0x00); // Horizontal addressing mode
	ssd1306_write(module, true, 0xA1); // Set segment remap with column address 127 mapped to segment 0
	ssd1306_write(module, true, 0xC8); // Set com output scan direction, scan from com63 to com 0
	ssd1306_write(module, true, 0xDA); // Set com pins hardware configuration
	ssd1306_write(module, true, 0x12); // Alternative com pin configuration, disable com left/right remap
	ssd1306_write(module, true, 0x81); // Set contrast control
	ssd1306_write(module, true, 0x80); // Set Contrast to 128
	ssd1306_write(module, true, 0xD9); // Set pre-charge period
	ssd1306_write(module, true, 0xF1); // Phase 1 period of 15 DCLK, Phase 2 period of 1 DCLK
	ssd1306_write(module, true, 0xDB); // Set Vcomh deselect level
	ssd1306_write(module, true, 0x20); // Vcomh deselect level ~ 0.77 Vcc
	ssd1306_write(module, true, 0xA4); // Entire display ON, resume to RAM content display
	ssd1306_write(module, true, 0xA6); // Set Display in Normal Mode, 1 = ON, 0 = OFF
	ssd1306_write(module, true, 0x2E); // Deactivate scroll
	ssd1306_write(module, true, 0xAF); // Display ON in normal mode
	ssd1306_clear(module);

	return 0;
}

static void ssd1306_goto_next_line(struct ssd1306_spi_module *module)
{
	module->line_num++;
	module->line_num = (module->line_num & SSD1306_MAX_LINE);
	ssd1306_set_cursor(module, module->line_num, 0);
}

static void ssd1306_print_char(struct ssd1306_spi_module *module, unsigned char c)
{
	uint8_t data_byte;
	uint8_t temp = 0;

	if (((module->cursor_position + module->font_size) >= SSD1306_MAX_SEG) || (c == '\n'))
		ssd1306_goto_next_line(module);

	if (c != '\n') {
		c -= 0x20;
		do {
			data_byte = ssd1306_font[c][temp]; 
			ssd1306_write(module, false, data_byte);   
			module->cursor_position++;

			temp++;

		} while (temp < module->font_size);

		ssd1306_write(module, false, 0x00); 
		module->cursor_position++;
	}
}

static void ssd1306_print_string(struct ssd1306_spi_module *module, unsigned char *str)
{
	while (*str) {
		ssd1306_print_char(module, *str++);
	}
}

static int ssd1306_probe(struct spi_device *spi)
{
    int ret;
    struct ssd1306_spi_module *module;
	printk(KERN_INFO "HUNGPHAN ssd1306 probe\n");

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

	gpio_request(RST_PIN, "RE");
	gpio_request(DC_PIN, "DC");
	gpio_direction_output(RST_PIN, 0);
	gpio_direction_output(DC_PIN, 0);

    module = kmalloc(sizeof(*module), GFP_KERNEL);
	if (!module) {
		pr_err("kmalloc failed\n");
		return -1;
	}

    module->device = spi;
    module->line_num = 0;
    module->cursor_position = 0;
    module->font_size = SSD1306_DEF_FONT_SIZE;
	module->dev = mdev.m_cdev;
    spi_set_drvdata(spi, module);
	module1 = module;


    // // Reset the display
    // gpio_set_value(RST_PIN, 0);
    // mdelay(10);
    // gpio_set_value(RST_PIN, 1);

    // Initialize the display
    ret = ssd1306_init_display(module);
    if (ret) {
        dev_err(&spi->dev, "Failed to initialize display\n");
        return ret;
    }

    ssd1306_print_string(module, "Hello\nWorld\n");

    // Display "Hello"
    return 0;

rm_device:
    device_destroy(mdev.m_class, mdev.dev_num);
rm_class:
    class_destroy(mdev.m_class);
rm_device_numb:
    unregister_chrdev_region(mdev.dev_num, 1);
    return -1;
}

static int ssd1306_remove(struct spi_device *spi)
{
    struct ssd1306_spi_module *module = spi_get_drvdata(spi);

	ssd1306_print_string(module, "End!!!");
	msleep(1000);
	ssd1306_clear(module);
	ssd1306_write(module, true, 0xAE); // Entire Display OFF

	kfree(module);
	cdev_del(&mdev.m_cdev);
    device_destroy(mdev.m_class, mdev.dev_num);
    class_destroy(mdev.m_class);
    unregister_chrdev_region(mdev.dev_num, 1);
	printk(KERN_INFO "HUNGPHAN ssd1306 remove\n");
    return 0;
}

static const struct of_device_id ssd1306_of_match[] = {
    { .compatible = "ssd1306" },
    { }
};
MODULE_DEVICE_TABLE(of, ssd1306_of_match);

static struct spi_driver ssd1306_driver = {
    .driver = {
        .name = "ssd1306",
        .of_match_table = ssd1306_of_match,
    },
    .probe = ssd1306_probe,
    .remove = ssd1306_remove,
};

module_spi_driver(ssd1306_driver);

MODULE_DESCRIPTION("SSD1306 OLED Driver");
MODULE_AUTHOR("hung.phan duchungeden@gmail.com");
MODULE_LICENSE("GPL");

static int m_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device has been opened\n");
	struct ssd1306_spi_module *module = NULL;
	struct spi_device *spi;

    file->private_data = module1;
    return 0;
}

static int m_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t m_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
     struct ssd1306_spi_module *module = file->private_data;
	// struct spi_device *spi;
    char *kbuf;
    int ret;
	

    // Allocate buffer for kernel space
    kbuf = kmalloc(count + 1, GFP_KERNEL);
    if (!kbuf) {
        printk(KERN_ERR "Failed to allocate memory\n");
        return -ENOMEM;
    }

    // Copy data from userspace to kernel space
    if (copy_from_user(kbuf, buf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }


    // Null-terminate the string
    kbuf[count] = '\0';
	printk(KERN_INFO "m_write: received buffer: %s\n", kbuf);
	
    if (!module) {
        printk(KERN_INFO "m_write: module is null\n");
        return -ENOMEM;
    }


    // Clear the screen
    ssd1306_clear(module);


    // Print the string on the screen
    ssd1306_print_string(module, (unsigned char *)kbuf);


    // Free the allocated memory
    kfree(kbuf);

    // Return the number of bytes written
    return count;
}

static ssize_t  m_read(struct file *filp, char __user *user_buf, size_t size,loff_t *offset) {
    return size;
}
