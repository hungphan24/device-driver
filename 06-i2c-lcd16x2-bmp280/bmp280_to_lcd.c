#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BMP280_DEV "/dev/bmp280"
#define LCD_DEV "/dev/lcd16x2"
#define BUFFER_SIZE 32

int main() {
    int bmp_fd, lcd_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Mở device file bmp280 để đọc
    bmp_fd = open(BMP280_DEV, O_RDONLY);
    if (bmp_fd < 0) {
        perror("Failed to open bmp280");
        exit(EXIT_FAILURE);
    }

    // Mở device file lcd16x2 để ghi
    lcd_fd = open(LCD_DEV, O_WRONLY);
    if (lcd_fd < 0) {
        perror("Failed to open lcd16x2");
        close(bmp_fd);
        exit(EXIT_FAILURE);
    }

    // Đọc dữ liệu từ bmp280
    bytes_read = read(bmp_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Failed to read from bmp280");
        close(bmp_fd);
        close(lcd_fd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0'; // Đảm bảo chuỗi kết thúc

    // Ghi nguyên văn dữ liệu xuống lcd16x2
    if (write(lcd_fd, buffer, bytes_read) != bytes_read) {
        perror("Failed to write to lcd16x2");
    }

    // Đóng file descriptors
    close(bmp_fd);
    close(lcd_fd);

    printf("Data written to LCD: %s\n", buffer);
    return 0;
}
