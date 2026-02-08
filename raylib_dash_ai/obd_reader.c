#include "obd_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

// Initialize the OBD connection
bool OBD_Init(OBDConnection* conn, const char* device_path) {
    conn->connected = false;
    strncpy(conn->device_path, device_path, sizeof(conn->device_path) - 1);

    // Open serial port
    conn->fd = open(device_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (conn->fd == -1) {
        fprintf(stderr, "Error opening %s: %s\n", device_path, strerror(errno));
        return false;
    }

    // Configure serial port
    struct termios options;
    tcgetattr(conn->fd, &options);

    // Set baud rate to 38400 (ELM327 default)
    cfsetispeed(&options, B38400);
    cfsetospeed(&options, B38400);

    // 8N1 mode
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // Enable receiver and set local mode
    options.c_cflag |= (CLOCAL | CREAD);

    // Raw input mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    // Set timeout
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;  // 1 second timeout

    tcsetattr(conn->fd, TCSANOW, &options);
    tcflush(conn->fd, TCIOFLUSH);

    // Initialize ELM327
    char response[256];

    // Reset device
    if (!OBD_SendCommand(conn, "ATZ\r", response, sizeof(response))) {
        fprintf(stderr, "Failed to reset ELM327\n");
        close(conn->fd);
        return false;
    }
    usleep(100000); // Wait 100ms

    // Turn off echo
    OBD_SendCommand(conn, "ATE0\r", response, sizeof(response));

    // Set protocol to automatic
    OBD_SendCommand(conn, "ATSP0\r", response, sizeof(response));

    conn->connected = true;
    printf("OBD-II connected on %s\n", device_path);
    return true;
}

// Send command and receive response
bool OBD_SendCommand(OBDConnection* conn, const char* cmd, char* response, int response_size) {
    if (!conn->connected) return false;

    // Clear buffers
    tcflush(conn->fd, TCIOFLUSH);

    // Send command
    int len = strlen(cmd);
    int written = write(conn->fd, cmd, len);
    if (written != len) {
        fprintf(stderr, "Failed to write command\n");
        return false;
    }

    // Read response
    usleep(100000); // Wait 100ms for response

    int total = 0;
    int attempts = 0;
    memset(response, 0, response_size);

    while (total < response_size - 1 && attempts < 50) {
        int n = read(conn->fd, response + total, response_size - total - 1);
        if (n > 0) {
            total += n;
            // Check if we got the prompt character '>'
            if (strchr(response, '>') != NULL) {
                break;
            }
        }
        usleep(20000); // Wait 20ms between reads
        attempts++;
    }

    response[total] = '\0';
    return total > 0;
}

// Parse hex response (e.g., "41 0C 1A F8" -> bytes)
static bool ParseHexResponse(const char* response, unsigned char* bytes, int* num_bytes) {
    *num_bytes = 0;
    char temp[3] = {0};
    const char* ptr = response;

    while (*ptr) {
        // Skip non-hex characters
        while (*ptr && !(((*ptr >= '0') && (*ptr <= '9')) ||
                        ((*ptr >= 'A') && (*ptr <= 'F')) ||
                        ((*ptr >= 'a') && (*ptr <= 'f')))) {
            ptr++;
        }

        if (!*ptr) break;

        // Read two hex digits
        temp[0] = *ptr++;
        if (!*ptr) break;
        temp[1] = *ptr++;
        temp[2] = '\0';

        bytes[*num_bytes] = (unsigned char)strtol(temp, NULL, 16);
        (*num_bytes)++;

        if (*num_bytes >= 64) break;  // Safety limit
    }

    return *num_bytes > 0;
}

// Read RPM (PID 01 0C)
int OBD_ReadRPM(OBDConnection* conn) {
    char response[256];

    if (!OBD_SendCommand(conn, "010C\r", response, sizeof(response))) {
        return -1;
    }

    // Response format: "41 0C XX XX" where XXXX is RPM * 4
    unsigned char bytes[64];
    int num_bytes;

    if (!ParseHexResponse(response, bytes, &num_bytes)) {
        return -1;
    }

    // Check if response is valid (41 0C is the header)
    if (num_bytes < 4 || bytes[0] != 0x41 || bytes[1] != 0x0C) {
        return -1;
    }

    // RPM = ((A * 256) + B) / 4
    int rpm = ((bytes[2] * 256) + bytes[3]) / 4;
    return rpm;
}

// Read vehicle speed (PID 01 0D)
int OBD_ReadSpeed(OBDConnection* conn) {
    char response[256];

    if (!OBD_SendCommand(conn, "010D\r", response, sizeof(response))) {
        return -1;
    }

    unsigned char bytes[64];
    int num_bytes;

    if (!ParseHexResponse(response, bytes, &num_bytes)) {
        return -1;
    }

    if (num_bytes < 3 || bytes[0] != 0x41 || bytes[1] != 0x0D) {
        return -1;
    }

    return bytes[2];  // Speed in km/h
}

// Read coolant temperature (PID 01 05)
int OBD_ReadCoolantTemp(OBDConnection* conn) {
    char response[256];

    if (!OBD_SendCommand(conn, "0105\r", response, sizeof(response))) {
        return -1;
    }

    unsigned char bytes[64];
    int num_bytes;

    if (!ParseHexResponse(response, bytes, &num_bytes)) {
        return -1;
    }

    if (num_bytes < 3 || bytes[0] != 0x41 || bytes[1] != 0x05) {
        return -1;
    }

    return bytes[2] - 40;  // Temperature = A - 40
}

// Close connection
void OBD_Close(OBDConnection* conn) {
    if (conn->connected) {
        close(conn->fd);
        conn->connected = false;
        printf("OBD-II disconnected\n");
    }
}
