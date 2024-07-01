#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGE_COUNT 8

// Define GPIO pins
#define DC_PIN 48
#define RST_PIN 49

static struct spi_device *ssd1306_spi_device;

static const u8 ssd1306_init_sequence[] = {
    0xAE,             // Display off
    0xD5,      // Set memory addressing mode
    0x80,             // Set page start address for page addressing mode
    0xA8,             // Set COM output scan direction
    0x3F,             // Set low column address
    0xD3,             // Set high column address
    0x00,             // Set start line address
    0x40,       // Set contrast control
    0x8D,             // Set segment re-map 0 to 127
    0x14,             // Set normal display
    0x20,      // Set multiplex ratio (1 to 64)
    0x00,             // Disable entire display on
    0xA1,      // Set display offset
    0xC8,      // Set display clock divide ratio/oscillator frequency
	0xDA,
	0x12,
	0x81,
	0x80,
	0xD9,
	0xF1,
	0xDB,
	0x20,
	0xA4,
	0xA6,
	0x2E,
	0xAF
};

// Font data for "Hello"
static const u8 hello_data[8 * 6] = {
    0x00, 0x7C, 0x14, 0x14, 0x7C, 0x00,  // H
    0x00, 0x7C, 0x54, 0x54, 0x00, 0x00,  // e
    0x00, 0x7C, 0x50, 0x50, 0x00, 0x00,  // l
    0x00, 0x7C, 0x50, 0x50, 0x00, 0x00,  // l
    0x00, 0x3C, 0x44, 0x44, 0x3C, 0x00,  // o
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

static int ssd1306_init_display(struct spi_device *spi)
{
	gpio_set_value(RST_PIN, 0);
	udelay(2);
	gpio_set_value(RST_PIN, 1);
    return ssd1306_send_command(spi, ssd1306_init_sequence, sizeof(ssd1306_init_sequence));
}

static int ssd1306_display_hello(struct spi_device *spi)
{
    int i;
    u8 buf[SSD1306_WIDTH] = {0};

    for (i = 0; i < 6; i++) {
        memcpy(buf + i * 8, hello_data + i * 8, 8);
    }

    for (i = 0; i < SSD1306_PAGE_COUNT; i++) {
        u8 page_command[] = { 0xB0 + i, 0x00, 0x10 };
        ssd1306_send_command(spi, page_command, sizeof(page_command));
        ssd1306_send_data(spi, buf, SSD1306_WIDTH);
    }

    return 0;
}

static int ssd1306_probe(struct spi_device *spi)
{
    int ret;
	printk(KERN_INFO "HUNGPHAN ssd1306 probe\n");

	gpio_request(RST_PIN, "RE");
	gpio_request(DC_PIN, "DC");
	gpio_direction_output(RST_PIN, 0);
	gpio_direction_output(DC_PIN, 0);


    ssd1306_spi_device = spi;

    // // Reset the display
    // gpio_set_value(RST_PIN, 0);
    // mdelay(10);
    // gpio_set_value(RST_PIN, 1);

    // Initialize the display
    ret = ssd1306_init_display(spi);
    if (ret) {
        dev_err(&spi->dev, "Failed to initialize display\n");
        return ret;
    }

    // Display "Hello"
    return ssd1306_display_hello(spi);
}

static int ssd1306_remove(struct spi_device *spi)
{
    // Turn off the display
    u8 off_cmd = 0xAE;
    ssd1306_send_command(spi, &off_cmd, 1);
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
