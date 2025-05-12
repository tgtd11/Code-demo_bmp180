Hướng dẫn sử dụng Driver BMP180 cho Raspberry Pi 3B+

Giới thiệu
Driver này cung cấp khả năng đọc nhiệt độ và áp suất từ cảm biến BMP180 thông qua giao tiếp I2C trên Raspberry Pi. Driver bao gồm 2 module kernel và một ứng dụng test.

1/Yêu cầu hệ thống
Raspberry Pi với kernel Linux hỗ trợ module

Cảm biến BMP180 kết nối qua I2C

Đã bật I2C trên Raspberry Pi (sử dụng raspi-config)

Công cụ biên dịch kernel (nếu cần)

Cài đặt
Sao chép các file vào Raspberry Pi:

bmp180_driver.c: Driver chính cho BMP180

bmp180_ioctl.c: Module cung cấp giao tiếp ioctl

test_bmp.c: Ứng dụng test

Biên dịch các module:

bash
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
Nạp các module vào kernel:

bash
sudo insmod bmp180_driver.ko
sudo insmod bmp180_ioctl.ko
Biên dịch ứng dụng test:

bash
gcc test_bmp.c -o test_bmp
Sử dụng
Kiểm tra module đã được nạp:

bash
lsmod | grep bmp180
Kiểm tra thiết bị đã được tạo:

bash
ls -l /dev/bmp180
Chạy ứng dụng test:

bash
sudo ./test_bmp
Ứng dụng sẽ đọc và hiển thị nhiệt độ, áp suất mỗi 0.5 giây trong 10 giây.

Gỡ bỏ
Gỡ các module:

bash
sudo rmmod bmp180_ioctl
sudo rmmod bmp180_driver
Giải thích các file
bmp180_driver.c:

Module chính giao tiếp với cảm biến qua I2C

Đọc các tham số hiệu chuẩn từ cảm biến

Export các biến cần thiết cho module ioctl

bmp180_ioctl.c:

Tạo character device /dev/bmp180

Cung cấp các lệnh ioctl để đọc nhiệt độ và áp suất

Thực hiện các phép tính toán từ dữ liệu thô

test_bmp.c:

Ứng dụng mẫu sử dụng ioctl để đọc dữ liệu

Hiển thị nhiệt độ (độ C) và áp suất (hPa)

Lưu ý
Cần quyền root để truy cập thiết bị (sử dụng sudo)

Đảm bảo cảm biến được kết nối đúng cách trước khi nạp module

Nếu gặp lỗi, kiểm tra kernel log bằng lệnh dmesg

Nhiệt độ trả về có đơn vị 0.1°C (ví dụ: 250 = 25.0°C)

Áp suất trả về có đơn vị hPa

Xử lý sự cố
Nếu không thấy thiết bị /dev/bmp180:

Kiểm tra xem module đã được nạp thành công chưa

Kiểm tra kernel log bằng dmesg | grep bmp180

Nếu gặp lỗi khi đọc dữ liệu:

Kiểm tra kết nối I2C

Xác minh địa chỉ I2C của BMP180 (mặc định là 0x77)

Nếu module không nạp được:

Kiểm tra xem kernel headers đã được cài đặt chưa

Biên dịch lại module với phiên bản kernel phù hợp
