//Nguyễn Thái Phiên_21146495
//Phạm Đức Thái_21146151
//Nguyễn Thành Nhân_21146492 

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/delay.h>


// Tên thiết bị và lớp
#define DEVICE_NAME "bmp180"
#define CLASS_NAME  "bmp180_class"


// Mã ioctl
#define BMP180_IOCTL_MAGIC 'b'
#define BMP180_IOCTL_GET_TEMP     _IOR(BMP180_IOCTL_MAGIC, 1, int)
#define BMP180_IOCTL_GET_PRESSURE _IOR(BMP180_IOCTL_MAGIC, 2, int)


// Thanh ghi BMP180
#define BMP180_REG_CONTROL       0xF4
#define BMP180_REG_OUT_MSB       0xF6
#define BMP180_REG_OUT_LSB       0xF7
#define BMP180_CMD_TEMP          0x2E
#define BMP180_CMD_PRESSURE      0x34


// Dùng các biến được export từ driver chính
extern struct i2c_client *bmp180_client;
extern struct bmp180_calib_param {
    int16_t ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t b1, b2, mb, mc, md;
} calib;


// Biến điều khiển char device
static int major_number;
static struct class *bmp180_class = NULL;
static struct device *bmp180_device = NULL;


// tính toán nhiệt độ
static int bmp180_read_temperature(void)
{
    int ut, x1, x2, b5, temp;
    int msb, lsb;

    if (!bmp180_client) {
        printk(KERN_ERR "BMP180 client not initialized\n");
        return -ENODEV;
    }

    /* Trigger temperature measurement */
    if (i2c_smbus_write_byte_data(bmp180_client, BMP180_REG_CONTROL, BMP180_CMD_TEMP) < 0) {
        printk(KERN_ERR "Failed to trigger temperature measurement\n");
        return -EIO;
    }
    msleep(5);

    /* Read raw temperature (16-bit) */
    msb = i2c_smbus_read_byte_data(bmp180_client, BMP180_REG_OUT_MSB);
    lsb = i2c_smbus_read_byte_data(bmp180_client, BMP180_REG_OUT_LSB);
    if (msb < 0 || lsb < 0) {
        printk(KERN_ERR "Failed to read temperature data: msb=%d, lsb=%d\n", msb, lsb);
        return -EIO;
    }
    ut = (msb << 8) | lsb;
    printk(KERN_DEBUG "Raw temperature: ut=%d\n", ut);

    /* Temperature compensation (unit: 0.1°C) */
    x1 = ((ut - calib.ac6) * calib.ac5) >> 15;
    if (x1 + calib.md == 0) {
        printk(KERN_ERR "Division by zero in temperature calculation\n");
        return -EINVAL;
    }
    x2 = (calib.mc << 11) / (x1 + calib.md);
    b5 = x1 + x2;
    temp = (b5 + 8) >> 4;

    printk(KERN_DEBUG "Temperature calc: x1=%d, x2=%d, b5=%d, temp=%d\n", x1, x2, b5, temp);

    return temp;
}


// tính toán áp suất
static int bmp180_read_pressure(void)
{
    int ut, up, x1, x2, x3, b3, b5, b6, b7, p;
    unsigned int b4;
    int msb, lsb;
    int oss = 0; /* Oversampling setting = 0 */

    if (!bmp180_client) {
        printk(KERN_ERR "BMP180 client not initialized\n");
        return -ENODEV;
    }

    /* Read temperature first to calculate b5 */
    if (i2c_smbus_write_byte_data(bmp180_client, BMP180_REG_CONTROL, BMP180_CMD_TEMP) < 0) {
        printk(KERN_ERR "Failed to trigger temperature measurement\n");
        return -EIO;
    }
    msleep(5);

    msb = i2c_smbus_read_byte_data(bmp180_client, BMP180_REG_OUT_MSB);
    lsb = i2c_smbus_read_byte_data(bmp180_client, BMP180_REG_OUT_LSB);
    if (msb < 0 || lsb < 0) {
        printk(KERN_ERR "Failed to read temperature data: msb=%d, lsb=%d\n", msb, lsb);
        return -EIO;
    }
    ut = (msb << 8) | lsb;
    printk(KERN_DEBUG "Raw temperature for pressure: ut=%d\n", ut);

    x1 = ((ut - calib.ac6) * calib.ac5) >> 15;
    if (x1 + calib.md == 0) {
        printk(KERN_ERR "Division by zero in temperature calculation\n");
        return -EINVAL;
    }
    x2 = (calib.mc << 11) / (x1 + calib.md);
    b5 = x1 + x2;

    /* Trigger pressure measurement */
    if (i2c_smbus_write_byte_data(bmp180_client, BMP180_REG_CONTROL, BMP180_CMD_PRESSURE | (oss << 6)) < 0) {
        printk(KERN_ERR "Failed to trigger pressure measurement\n");
        return -EIO;
    }
    msleep(8); /* Wait time for OSS=0 */

    /* Read raw pressure (16-bit for OSS=0) */
    msb = i2c_smbus_read_byte_data(bmp180_client, BMP180_REG_OUT_MSB);
    lsb = i2c_smbus_read_byte_data(bmp180_client, BMP180_REG_OUT_LSB);
    if (msb < 0 || lsb < 0) {
        printk(KERN_ERR "Failed to read pressure data: msb=%d, lsb=%d\n", msb, lsb);
        return -EIO;
    }
    up = (msb << 8) | lsb;
    printk(KERN_DEBUG "Raw pressure: up=%d\n", up);

    /* Pressure compensation (unit: hPa) */
    b6 = b5 - 4000;
    x1 = (calib.b2 * ((b6 * b6) >> 12)) >> 11;
    x2 = (calib.ac2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = (((calib.ac1 * 4 + x3) << oss) + 2) / 4;
    x1 = (calib.ac3 * b6) >> 13;
    x2 = (calib.b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (calib.ac4 * (unsigned long)(x3 + 32768)) >> 15;
    b7 = ((unsigned long)up - b3) * (50000 >> oss);

    if (b7 < 0x80000000)
        p = (b7 * 2) / b4;
    else
        p = (b7 / b4) * 2;

    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p = p + ((x1 + x2 + 3791) >> 4);

    printk(KERN_DEBUG "Pressure calc: b6=%d, b3=%d, b4=%u, b7=%d, p=%d\n", b6, b3, b4, b7, p);

    return p / 100; /* Convert to hPa */
}

//IOCTL
static long bmp180_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int data;

    switch (cmd) {
    case BMP180_IOCTL_GET_TEMP:
        data = bmp180_read_temperature();
        break;
    case BMP180_IOCTL_GET_PRESSURE:
        data = bmp180_read_pressure();
        break;
    default:
        return -EINVAL;
    }

    if (data < 0)
        return data;

    if (copy_to_user((int __user *)arg, &data, sizeof(data)))
        return -EFAULT;

    return 0;
}

static int bmp180_open(struct inode *inodep, struct file *filep)
{
    return 0;
}

static int bmp180_release(struct inode *inodep, struct file *filep)
{
    return 0;
}


// Ðang ký file operations
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = bmp180_ioctl,
    .open = bmp180_open,
    .release = bmp180_release,
};

// Hàm kh?i t?o module ioctl
static int __init bmp180_ioctl_init(void)
{
    if (!bmp180_client) {
        printk(KERN_ERR "BMP180 driver not loaded or client not initialized\n");
        return -ENODEV;
    }
// Ðang ký char device và t?o file /dev/bmp180
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register char device: %d\n", major_number);
        return major_number;
    }

    bmp180_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(bmp180_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(bmp180_class);
    }

    bmp180_device = device_create(bmp180_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(bmp180_device)) {
        class_destroy(bmp180_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(bmp180_device);
    }

    printk(KERN_INFO "BMP180 ioctl module loaded\n");
    return 0;
}
// Hàm gỡ module
static void __exit bmp180_ioctl_exit(void)
{
    device_destroy(bmp180_class, MKDEV(major_number, 0));
    class_unregister(bmp180_class);
    class_destroy(bmp180_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "BMP180 ioctl module unloaded\n");
}

module_init(bmp180_ioctl_init);
module_exit(bmp180_ioctl_exit);

MODULE_AUTHOR("phien_raspberry");
MODULE_DESCRIPTION("BMP180 ioctl_demo code");
MODULE_LICENSE("GPL");
