//Nguyễn Thái Phiên_21146495
//Phạm Đức Thái_21146151
//Nguyễn Thành Nhân_21146492




#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/of.h>

// Địa chỉ thanh ghi và mã chip ID của BMP180
#define BMP180_CHIP_ID_REG 0xD0
#define BMP180_CHIP_ID     0x55




// Cấu trúc lưu các tham số hiệu chuẩn của BMP180
struct bmp180_calib_param {
    int16_t ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t b1, b2, mb, mc, md;
};

// Biến client đại diện cho thiết bị I2C, và hệ số hiệu chuẩn
struct i2c_client *bmp180_client;
struct bmp180_calib_param calib;

// Cho phép module khác truy c?p
EXPORT_SYMBOL(bmp180_client);
EXPORT_SYMBOL(calib);


// Hàm đọc các tham số hiệu chuẩn từ thanh ghi
static int bmp180_read_calibration_data(struct i2c_client *client)
{
    calib.ac1 = swab16(i2c_smbus_read_word_data(client, 0xAA));     //  d?c thanh ghi
    calib.ac2 = swab16(i2c_smbus_read_word_data(client, 0xAC));
    calib.ac3 = swab16(i2c_smbus_read_word_data(client, 0xAE));
    calib.ac4 = swab16(i2c_smbus_read_word_data(client, 0xB0));
    calib.ac5 = swab16(i2c_smbus_read_word_data(client, 0xB2));
    calib.ac6 = swab16(i2c_smbus_read_word_data(client, 0xB4));
    calib.b1 = swab16(i2c_smbus_read_word_data(client, 0xB6));
    calib.b2 = swab16(i2c_smbus_read_word_data(client, 0xB8));
    calib.mb = swab16(i2c_smbus_read_word_data(client, 0xBA));
    calib.mc = swab16(i2c_smbus_read_word_data(client, 0xBC));
    calib.md = swab16(i2c_smbus_read_word_data(client, 0xBE));

// Kiểm tra tính hợp lệ
    if (calib.ac1 < 0 || calib.ac2 < 0 || calib.ac3 < 0 ||
        calib.ac4 == 0 || calib.ac5 == 0 || calib.ac6 == 0 ||
        calib.b1 < 0 || calib.b2 < 0 || calib.mb < 0 ||
        calib.mc < 0 || calib.md < 0) {
        dev_err(&client->dev, "Failed to read calibration data\n");
        return -EIO;
    }

    return 0;
}


static int bmp180_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int chip_id;
	// Lưu client vào biến toàn cục
    bmp180_client = client;

    // Đọc chip ID từ thanh ghi
    chip_id = i2c_smbus_read_byte_data(client, BMP180_CHIP_ID_REG);
    if (chip_id < 0) {
        dev_err(&client->dev, "Failed to read chip ID\n");
        return chip_id;
    }
	// Kiểm tra chip ID
    if (chip_id != BMP180_CHIP_ID) {
        dev_err(&client->dev, "Invalid chip ID: 0x%x\n", chip_id);
        return -ENODEV;
    }

    if (bmp180_read_calibration_data(client)) {
        dev_err(&client->dev, "Failed to read calibration parameters\n");
        return -EIO;
    }

    // In tham số hiệu chuẩn để kiểm tra
    dev_info(&client->dev, "Calibration data: ac1=%d, ac2=%d, ac3=%d, ac4=%u, ac5=%u, ac6=%u, b1=%d, b2=%d, mb=%d, mc=%d, md=%d\n",
             calib.ac1, calib.ac2, calib.ac3, calib.ac4, calib.ac5, calib.ac6,
             calib.b1, calib.b2, calib.mb, calib.mc, calib.md);

    dev_info(&client->dev, "BMP180 detected with chip ID 0x%x\n", chip_id);
    return 0;
}
// Hàm remove
static void bmp180_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "BMP180 driver removed\n");
}
// Bảng ID thiết bị I2C
static const struct i2c_device_id bmp180_id[] = {
    { "bmp180", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, bmp180_id);

// Thiết bị hỗ trợ thông qua device tree
static const struct of_device_id bmp180_of_match[] = {
    { .compatible = "bosch,bmp180" },
    { }
};
MODULE_DEVICE_TABLE(of, bmp180_of_match);



// Ðăng ký driver với kernel
static struct i2c_driver bmp180_driver = {
    .driver = {
        .name = "bmp180",
        .of_match_table = bmp180_of_match,
    },
    .probe = bmp180_probe,
    .remove = bmp180_remove,
    .id_table = bmp180_id,
};

module_i2c_driver(bmp180_driver);

MODULE_AUTHOR("phien_raspberry");
MODULE_DESCRIPTION("BMP180 Driver_demo code");
MODULE_LICENSE("GPL");
