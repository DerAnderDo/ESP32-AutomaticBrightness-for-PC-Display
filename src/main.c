#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <math.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define BUFFER_SIZE 256

int open_serial_port(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("Error opening serial port");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void configure_serial_port(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("Error from tcgetattr");
        exit(EXIT_FAILURE);
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= IGNBRK;
    tty.c_iflag &= IGNCR;
    tty.c_iflag &= INLCR;
    tty.c_iflag &= ICRNL;

    tty.c_oflag &= ~OPOST;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        exit(EXIT_FAILURE);
    }
}

float extract_lux_value(const char *data) {
    // Check if the pattern "lux: " followed by a float value is present in the data
    const char *luxToken = strstr(data, "lux: ");
    if (luxToken != NULL) {
        // If the pattern is found, try to extract the float value
        float luxValue;
        int matches = sscanf(luxToken, "lux: %f", &luxValue);
        if (matches == 1) {
            // Return the extracted lux value
            return luxValue;
        }
    }

    // Return a default value if the pattern is not found or extraction fails
    return -1.0;
}

void read_and_process_serial_data(int fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    float lastSavedLuxValue = -1.0;

    while (1) {
        bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            //printf("Received: %s", buffer);

            // Extract the lux value
            float luxValue = extract_lux_value(buffer);
            if (luxValue != -1.0) {
                // Perform the division
                float newValue = luxValue / 15.0;

                // Convert to integer and limit to 100
                int intValue = (int)(newValue + 0.5); // Round to the nearest integer
                intValue = (intValue > 100) ? 100 : intValue;

                // Check if the difference is greater than or equal to 5
                if (fabs(newValue - lastSavedLuxValue) >= 5.0) {
                    // Update the last saved value
                    lastSavedLuxValue = newValue;

                    // Run the Linux command using the new value
                    char command[256];
                    snprintf(command, sizeof(command), "ddcutil --display 1 setvcp 10 %d", intValue);
                    int result = system(command);

                    if (result == 0) {
                        printf("Set brightness to: %d\n", intValue);
                    } else {
                        fprintf(stderr, "Error executing command\n");
                    }
                }
            }
        } else if (bytesRead < 0) {
            perror("Error reading from serial port");
            exit(EXIT_FAILURE);
        }
    }
}

int main() {
    int serial_fd = open_serial_port(SERIAL_PORT);
    configure_serial_port(serial_fd);

    read_and_process_serial_data(serial_fd);

    close(serial_fd);

    return 0;
}
