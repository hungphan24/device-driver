#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/device.h>

#define BMP280_CHIP_ID_REG 0xD0
#define BMP280_CHIP_ID 0x58
#define BMP280_CALIB_START 0x88
#define BMP280_CALIB_END 0xA1
#define BMP280_REG_PRESS_MSB 0xF7 // Thanh ghi MSB áp suất
#define BMP280_REG_TEMP_MSB 0xFA  // Thanh ghi MSB nhiệt độ

// Cấu trúc lưu hằng số hiệu chỉnh
struct bmp280_calib {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
};

struct bmp280_data {
    struct i2c_client *client;
    struct cdev cdev;
    dev_t dev_num;
    struct class *class;
    struct bmp280_calib calib;
};

static int bmp280_read_calib(struct i2c_client *client) {
    struct bmp280_data *data = i2c_get_clientdata(client);
    uint8_t calib_data[24];
    int ret;

    if (!data) {
        pr_err("bmp280_data is NULL\n");
        return -EINVAL;
    }

    ret = i2c_smbus_read_i2c_block_data(client, BMP280_CALIB_START, 24, calib_data);
    if (ret < 0) {
        pr_err("Failed to read calibration data\n");
        return ret;
    }

    data->calib.dig_T1 = (calib_data[1] << 8) | calib_data[0];
    data->calib.dig_T2 = (calib_data[3] << 8) | calib_data[2];
    data->calib.dig_T3 = (calib_data[5] << 8) | calib_data[4];
    data->calib.dig_P1 = (calib_data[7] << 8) | calib_data[6];
    data->calib.dig_P2 = (calib_data[9] << 8) | calib_data[8];
    data->calib.dig_P3 = (calib_data[11] << 8) | calib_data[10];
    data->calib.dig_P4 = (calib_data[13] << 8) | calib_data[12];
    data->calib.dig_P5 = (calib_data[15] << 8) | calib_data[14];
    data->calib.dig_P6 = (calib_data[17] << 8) | calib_data[16];
    data->calib.dig_P7 = (calib_data[19] << 8) | calib_data[18];
    data->calib.dig_P8 = (calib_data[21] << 8) | calib_data[20];
    data->calib.dig_P9 = (calib_data[23] << 8) | calib_data[22];
    return 0;
}

static void bmp280_compensate(struct bmp280_data *data, int32_t adc_T, int32_t adc_P, int32_t *t_fine, int32_t *temperature, int32_t *pressure) {
    int32_t var1, var2;

    // Bù nhiệt độ
    var1 = ((((adc_T >> 3) - ((int32_t)data->calib.dig_T1 << 1))) * ((int32_t)data->calib.dig_T2)) >> 11;
    var2 = ((((adc_T >> 4) - ((int32_t)data->calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)data->calib.dig_T1))) >> 12) * ((int32_t)data->calib.dig_T3);
    *t_fine = var1 + var2;
    *temperature = (*t_fine * 5 + 128) >> 8; // 0.01°C

    // Bù áp suất
    var1 = (*t_fine >> 1) - 64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)data->calib.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)data->calib.dig_P5)) << 1);
    var2 = (var2 >> 2) + ((int32_t)data->calib.dig_P4 << 16);
    var1 = (((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)data->calib.dig_P3 << 5) + ((var1 >> 2) * (int32_t)data->calib.dig_P2);
    var1 = var1 >> 1;
    var1 = var1 + ((int32_t)data->calib.dig_P1 << 8);
    var1 = var1 >> 4;
    *pressure = (((adc_P >> 2) - (var2 >> 11)) * 3125);
    if (*pressure < 0x80000000) {
        *pressure = (*pressure << 1) / var1;
    } else {
        *pressure = (*pressure / var1) * 2;
    }
    var1 = ((int32_t)data->calib.dig_P9 * (((*pressure >> 3) * (*pressure >> 3)) >> 13)) >> 12;
    var2 = ((int32_t)data->calib.dig_P8 * (*pressure)) >> 13;
    *pressure = *pressure + ((var1 + var2 + ((int32_t)data->calib.dig_P7)) >> 4);
}

static ssize_t bmp280_read(struct file *file, char __user *buf, size_t count, loff_t *off) {
    struct bmp280_data *data = container_of(file->f_inode->i_cdev, struct bmp280_data, cdev);
    uint8_t data_buf[6];
    int32_t adc_press, adc_temp, t_fine, temp, press;
    int ret;
    
    if (*off > 0) {
        return 0;
    }

    ret = i2c_smbus_read_i2c_block_data(data->client, BMP280_REG_PRESS_MSB, 6, data_buf);
    if (ret < 0) {
        pr_err("Failed to read BMP280 data\n");
        return ret;
    }

    adc_press = (data_buf[0] << 12) | (data_buf[1] << 4) | (data_buf[2] >> 4);
    adc_temp = (data_buf[3] << 12) | (data_buf[4] << 4) | (data_buf[5] >> 4);

    bmp280_compensate(data, adc_temp, adc_press, &t_fine, &temp, &press);

    char kbuf[32] = {0};
    int len = snprintf(kbuf, 32, "Temp: %dC, Press: %dPa\n", temp / 100, press);
    if (copy_to_user(buf, kbuf, strlen(kbuf) + 1)) {
        pr_err("Failed to copy to user\n");
        return -EFAULT;
    }

    *off += len;

    return strlen(kbuf) + 1;
}

static int bmp280_open(struct inode *inode, struct file *file) {
    struct bmp280_data *data = container_of(inode->i_cdev, struct bmp280_data, cdev);
    file->private_data = data;
    pr_info("BMP280 device opened\n");
    return 0;
}

static int bmp280_release(struct inode *inode, struct file *file) {
    pr_info("BMP280 device closed\n");
    return 0;
}

static struct file_operations bmp280_fops = {
    .owner = THIS_MODULE,
    .open = bmp280_open,
    .release = bmp280_release,
    .read = bmp280_read,
};

static int bmp280_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    struct bmp280_data *data;
    struct device_node *np = client->dev.of_node;
    int ret;
    uint8_t chip_id;

    if (!np) {
        pr_err("Device Tree node not found\n");
        return -ENODEV;
    }

    // Cấp phát bộ nhớ cho data trước
    data = devm_kzalloc(&client->dev, sizeof(struct bmp280_data), GFP_KERNEL);
    if (!data) {
        pr_err("Failed to allocate memory for bmp280_data\n");
        return -ENOMEM;
    }
    data->client = client;
    i2c_set_clientdata(client, data);

    // Kiểm tra ID chip
    ret = i2c_smbus_read_byte_data(client, BMP280_CHIP_ID_REG);
    if (ret < 0 || ret != BMP280_CHIP_ID) {
        pr_err("BMP280 not detected, wrong chip ID: 0x%x\n", ret);
        return -ENODEV;
    }
    chip_id = ret;
    pr_info("BMP280 detected, Chip ID: 0x%x\n", chip_id);

    // Đọc hằng số hiệu chỉnh
    ret = bmp280_read_calib(client);
    if (ret < 0) return ret;

    // Đăng ký character device
    ret = alloc_chrdev_region(&data->dev_num, 0, 1, "bmp280");
    if (ret < 0) {
        pr_err("Failed to allocate chrdev region\n");
        return ret;
    }

    cdev_init(&data->cdev, &bmp280_fops);
    ret = cdev_add(&data->cdev, data->dev_num, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        goto unregister_chrdev;
    }

    data->class = class_create(THIS_MODULE, "bmp280");
    if (IS_ERR(data->class)) {
        pr_err("Failed to create class\n");
        ret = PTR_ERR(data->class);
        goto del_cdev;
    }

    device_create(data->class, NULL, data->dev_num, NULL, "bmp280");
    pr_info("BMP280 driver loaded, device file: /dev/bmp280\n");

    return 0;

del_cdev:
    cdev_del(&data->cdev);
unregister_chrdev:
    unregister_chrdev_region(data->dev_num, 1);
    return ret;
}

static int bmp280_remove(struct i2c_client *client) {
    struct bmp280_data *data = i2c_get_clientdata(client);

    device_destroy(data->class, data->dev_num);
    class_destroy(data->class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->dev_num, 1);
    pr_info("BMP280 driver removed\n");
    return 0;
}

static const struct of_device_id bmp280_of_match[] = {
    { .compatible = "bosch,bmp280" },
    { }
};
MODULE_DEVICE_TABLE(of, bmp280_of_match);

static struct i2c_driver bmp280_driver = {
    .driver = {
        .name = "bmp280",
        .of_match_table = of_match_ptr(bmp280_of_match),
    },
    .probe = bmp280_probe,
    .remove = bmp280_remove,
};

module_i2c_driver(bmp280_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("BMP280 I2C Driver with Device Tree");
