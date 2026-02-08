#ifndef OBD_READER_H
#define OBD_READER_H

#include <stdbool.h>

typedef struct {
    int fd;                  // File descriptor for serial port
    bool connected;
    char device_path[256];   // e.g., "/dev/tty.OBD-II-Port" or "COM3"
} OBDConnection;

// Initialize OBD connection
bool OBD_Init(OBDConnection* conn, const char* device_path);

// Send command and get response
bool OBD_SendCommand(OBDConnection* conn, const char* cmd, char* response, int response_size);

// Read RPM from vehicle (returns RPM value, or -1 on error)
int OBD_ReadRPM(OBDConnection* conn);

// Read vehicle speed in km/h
int OBD_ReadSpeed(OBDConnection* conn);

// Read coolant temperature in Celsius
int OBD_ReadCoolantTemp(OBDConnection* conn);

// Close OBD connection
void OBD_Close(OBDConnection* conn);

#endif // OBD_READER_H
