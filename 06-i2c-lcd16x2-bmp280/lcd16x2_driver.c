#include <linux/module.h>

#include <linux/init.h>

#include <linux/slab.h>

#include <linux/i2c.h>

#include <linux/delay.h>

#include <linux/kernel.h>

#include <linux/device.h>
#include <linux/fs.h>         // file_operations
#include <linux/uaccess.h>    // copy_to/from_user
#include <linux/device.h>     // class_create, device_create
#include <linux/cdev.h>       // cdev


static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;
#define DEVICE_NAME "lcd16x2"
#define MINOR_COUNT 1

#define LCD_BACKLIGHT  0x08  // Đèn nền ON
#define ENABLE         0x04  // EN = bit 2
#define READ_WRITE     0x02  // RW = bit 1
#define REGISTER_SEL   0x01  // RS = bit 0

static struct i2c_client *lcd_client;

static int lcd_send_command(struct i2c_client *client, uint8_t command);
static int lcd_send_data(struct i2c_client *client, uint8_t data);



static int my_open(struct inode *inode, struct file *file)
{
    pr_info("Device opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("Device closed\n");
    return 0;
}


static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *off)
{
    char kbuf[32];
    int i, ret;

    if (!lcd_client) {
        pr_err("I2C client not initialized\n");
        return -ENODEV;
    }

    if (count > 31) count = 31; 
    if (copy_from_user(kbuf, buf, count)) {
        return -EFAULT;
    }
    kbuf[count] = '\0';

    
    lcd_send_command(lcd_client, 0x01); 
    msleep(5);
    lcd_send_command(lcd_client, 0x80);
    msleep(5);

    
    for (i = 0; i < count && kbuf[i] != '\0' && i < 16; i++) { 
        ret = lcd_send_data(lcd_client, kbuf[i]);
        if (ret < 0) {
            pr_err("Failed to send data to LCD\n");
            return ret;
        }
    }

    pr_info("Displayed on LCD: %s\n", kbuf);
    return count;
}

static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .write = my_write,
};

static void lcd_send_nibble(uint8_t nibble, uint8_t control)
{
    uint8_t data;

    // Gửi nibble (4 bit dữ liệu ở bit 4~7), OR với control (RS, RW, EN)
    data = (nibble & 0xF0) | control | LCD_BACKLIGHT;

    // Enable pulse
    i2c_master_send(lcd_client, &data, 1);
    udelay(1);
    data |= ENABLE;
    i2c_master_send(lcd_client, &data, 1);
    udelay(1);
    data &= ~ENABLE;
    i2c_master_send(lcd_client, &data, 1);
    udelay(100);
}

static void lcd_write_byte(uint8_t byte, uint8_t control)
{
    lcd_send_nibble(byte & 0xF0, control);        // high nibble
    lcd_send_nibble((byte << 4) & 0xF0, control); // low nibble
}


static int lcd_send_command(struct i2c_client *client, uint8_t command)

{
    lcd_write_byte(command, 0x00);  // RS=0, RW=0
    return 0;
    
}



static int lcd_send_data(struct i2c_client *client, uint8_t data)

{

   lcd_write_byte(data, REGISTER_SEL); // RS=1, RW=0
    return 0;

}



static void lcd_init(struct i2c_client *client)

{

   msleep(50);  // Đợi LCD lên nguồn

    // Gửi 0x30 3 lần theo datasheet
    lcd_send_nibble(0x30, 0);
    msleep(5);
    lcd_send_nibble(0x30, 0);
    udelay(150);
    lcd_send_nibble(0x30, 0);
    msleep(1);

    lcd_send_nibble(0x20, 0); // Chuyển sang chế độ 4-bit
    msleep(1);

    lcd_send_command(client, 0x28); // 4-bit, 2 dòng, 5x8 font
    lcd_send_command(client, 0x08); // Display OFF
    lcd_send_command(client, 0x01); // Clear
    msleep(2);
    lcd_send_command(client, 0x06); // Entry mode
    lcd_send_command(client, 0x0C); // Display ON, cursor OFF

}



static int lcd_display_string(struct i2c_client *client, const char *str)

{

    while (*str)

    {

        lcd_send_data(client, *str++);

    }

    return 0;

}

static int lcd16x2_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    lcd_client = client;
    lcd_init(client);

    lcd_display_string(client, "hello");

    struct device_node *np = client->dev.of_node;

    uint32_t reg;
    uint32_t ret;



    if (!np) {

        dev_err(&client->dev, "Device tree node not found\n");

        return -ENODEV;

    }



    if (of_property_read_u32(np, "reg", &reg)) {

        dev_err(&client->dev, "Failed to read reg property\n");

        return -EINVAL;

    }

    // Khởi tạo character driver
    ret = alloc_chrdev_region(&dev_num, 0, MINOR_COUNT, DEVICE_NAME);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to allocate chrdev region\n");
        lcd_client = NULL;
        return ret;
    }
    dev_info(&client->dev, "Major number: %d\n", MAJOR(dev_num));

    cdev_init(&my_cdev, &my_fops);
    ret = cdev_add(&my_cdev, dev_num, MINOR_COUNT);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to add cdev\n");
        goto unregister;
    }

    my_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(my_class)) {
        dev_err(&client->dev, "Failed to create class\n");
        ret = PTR_ERR(my_class);
        goto cdev_del;
    }

    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        dev_err(&client->dev, "Failed to create device\n");
        ret = PTR_ERR(my_device);
        goto class_destroy;
    }    



    dev_info(&client->dev, "reg property of lcd16x2 node: 0x%x\n", reg);



    pr_info("LCD driver probed\n");

    return 0;


class_destroy:
    class_destroy(my_class);
cdev_del:
    cdev_del(&my_cdev);
unregister:
    unregister_chrdev_region(dev_num, MINOR_COUNT);
    lcd_client = NULL;
    return ret;
}

static int lcd16x2_i2c_remove(struct i2c_client *client) {

    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, MINOR_COUNT);
    lcd_send_command(client, 0x01); // Clear display
    lcd_client = NULL; // Xóa client khi remove
    pr_info("LCD driver removed\n");

    return 0;

} 

static const struct of_device_id lcd16x2_of_match[] = {
    { .compatible = "ti,pcf8574" },
    { }
};
MODULE_DEVICE_TABLE(of, lcd16x2_of_match);

static struct i2c_driver lcd16x2_i2c_driver = {
	.driver = {
		.name = "ti,pcf8574",
		.owner = THIS_MODULE,
		.of_match_table = lcd16x2_of_match,
	},
	.probe = lcd16x2_i2c_probe,
	.remove = lcd16x2_i2c_remove,
};

module_i2c_driver(lcd16x2_i2c_driver);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("hung.phan");
MODULE_DESCRIPTION("lcd16x2");


