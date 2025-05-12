#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

// Ðu?ng d?n thi?t b? character driver
#define DEVICE_PATH "/dev/bmp180"

// Mã ioctl
#define BMP180_IOCTL_MAGIC 'b'
#define BMP180_IOCTL_GET_TEMP     _IOR(BMP180_IOCTL_MAGIC, 1, int)
#define BMP180_IOCTL_GET_PRESSURE _IOR(BMP180_IOCTL_MAGIC, 2, int)

int main() {
    int fd;
    int temp, pressure;
    time_t start_time, current_time;

    // Mo thiet bi
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Khong the mo thiet bi BMP180");
        return errno;
    }

    // Lay thoi gian bat dau
    start_time = time(NULL);
    
    printf("Bat dau do lien tuc trong 10 giay...\n");

    // Vong lap do trong 10 giay
    while (1) {
        current_time = time(NULL);
        if (current_time - start_time >= 10) {
            break; // Thoat sau 10 giay
        }

        // Doc nhiet do
        if (ioctl(fd, BMP180_IOCTL_GET_TEMP, &temp) < 0) {
            perror("Khong the doc nhiet do");
            close(fd);
            return errno;
        }

        // Doc ap suat
        if (ioctl(fd, BMP180_IOCTL_GET_PRESSURE, &pressure) < 0) {
            perror("Khong the doc ap suat");
            close(fd);
            return errno;
        }

        // In ket qua
        printf("Thoi gian: %ld giay, Nhiet do: %.1f do C, Ap suat: %d hPa\n", 
               current_time - start_time, temp / 10.0, pressure);

        // Doi 0.5 giay truoc khi do tiep
        usleep(500000); // 500000 microgiay = 0.5 giay
    }

    // Dong thiet bi
    close(fd);
    printf("Hoan tat do.\n");
    return 0;
}