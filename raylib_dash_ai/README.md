# OBD-II Car Tachometer

A real-time car tachometer display built with C and raylib that can read live data from your vehicle's OBD-II port using an ELM327 adapter.

![Tachometer Preview](https://img.shields.io/badge/Language-C-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

## Features

- **Dual Mode Operation**
  - 🎮 **Simulation Mode**: Test with keyboard controls (UP/DOWN arrows)
  - 🚗 **OBD Mode**: Live data from your vehicle via ELM327 adapter

- **Visual Elements**
  - Circular analog gauge (0-8000 RPM)
  - Color-coded zones: Green (safe), Yellow (high), Red (redline 7000+ RPM)
  - Smooth animated needle with realistic movement
  - Digital RPM readout display
  - Redline warning indicator

- **OBD-II Integration**
  - ELM327 serial communication
  - Real-time RPM reading (PID 01 0C)
  - Non-blocking threaded data acquisition
  - Support for additional sensors (speed, coolant temp)

## Hardware Requirements

### For Simulation Mode
- Any computer with OpenGL support

### For OBD Mode
- **ELM327 OBD-II Adapter** (Bluetooth, USB, or WiFi)
  - Recommended: USB or Bluetooth versions (~$10-20)
  - Must support standard OBD-II protocols
- Vehicle with OBD-II port (1996+ cars in USA, 2001+ in EU)

## Software Dependencies

### Required Libraries
- **raylib** - Graphics library for the tachometer UI
- **pthread** - POSIX threads for non-blocking OBD reading
- **termios** - Serial port communication (POSIX systems)

### macOS Installation
```bash
# Install raylib via Homebrew
brew install raylib

# pthread and termios are included in macOS
```

### Linux Installation
```bash
# Debian/Ubuntu
sudo apt install libraylib-dev

# Fedora/RHEL
sudo dnf install raylib-devel

# pthread is usually included in glibc
```

## Project Structure

```
raylib_tach/
├── README.md                  # This file
├── tachometer.c              # Basic tachometer (simulation only)
├── tachometer_obd.c          # Tachometer with OBD-II support
├── obd_reader.h              # OBD-II interface header
├── obd_reader.c              # OBD-II implementation (ELM327)
└── libraylib.a               # Compiled raylib library
```

## Compilation

### Simulation-Only Tachometer
```bash
cd raylib_tach
gcc tachometer.c -o tachometer -L. -lraylib \
    -framework CoreVideo -framework IOKit \
    -framework Cocoa -framework OpenGL
```

### OBD-II Enabled Tachometer
```bash
cd raylib_tach
gcc tachometer_obd.c obd_reader.c -o tachometer_obd -L. -lraylib \
    -framework CoreVideo -framework IOKit \
    -framework Cocoa -framework OpenGL -lpthread
```

#### Linux Compilation
```bash
# Simulation only
gcc tachometer.c -o tachometer -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# With OBD support
gcc tachometer_obd.c obd_reader.c -o tachometer_obd \
    -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```

## Usage

### Simulation Mode

Run the basic tachometer:
```bash
./tachometer
```

**Controls:**
- `UP ARROW` - Increase RPM
- `DOWN ARROW` - Decrease RPM
- `ESC` or close window - Exit

### OBD-II Mode

#### 1. Connect Your ELM327 Adapter

**Bluetooth Setup (macOS):**
1. Pair your ELM327 adapter via System Preferences > Bluetooth
2. Note the device name (usually contains "OBD" or "ELM327")
3. Find the device path:
   ```bash
   ls /dev/tty.* | grep -i obd
   # or
   ls /dev/tty.* | grep -i elm
   ```
   Example output: `/dev/tty.OBD-II-Port` or `/dev/tty.ELM327-DevB`

**USB Setup:**
```bash
# macOS
ls /dev/tty.usbserial*
# Example: /dev/tty.usbserial-1420

# Linux
ls /dev/ttyUSB*
# Example: /dev/ttyUSB0
```

**Bluetooth Setup (Linux):**
```bash
# Pair device first, then create serial connection
sudo rfcomm bind /dev/rfcomm0 <MAC_ADDRESS> 1
# Example: sudo rfcomm bind /dev/rfcomm0 00:1D:A5:12:34:56 1
```

#### 2. Configure Device Path

Edit `tachometer_obd.c` line 235 and set your device path:
```c
if (OBD_Init(&tach.obd, "/dev/tty.OBD-II-Port")) {
    // Change to your device path, e.g.:
    // "/dev/tty.usbserial-1420"  (macOS USB)
    // "/dev/ttyUSB0"             (Linux USB)
    // "/dev/rfcomm0"             (Linux Bluetooth)
```

#### 3. Run the Program

```bash
./tachometer_obd
```

**Controls:**
- `O` - Toggle between Simulation and OBD mode
- `UP/DOWN ARROWS` - Control RPM (simulation mode only)
- `ESC` or close window - Exit

#### 4. Connect to Vehicle

1. Start the program (begins in simulation mode)
2. Connect your ELM327 adapter to the vehicle's OBD-II port
3. Turn on vehicle ignition (engine can be off)
4. Press `O` to switch to OBD mode
5. The tachometer will display live RPM data
6. Start the engine to see real-time readings

## Troubleshooting

### "Cannot open device"
- **Check permissions:**
  ```bash
  # macOS/Linux - add user to dialout group
  sudo usermod -a -G dialout $USER
  # Log out and back in

  # Or use sudo (not recommended for regular use)
  sudo ./tachometer_obd
  ```
- **Verify device path:** Run `ls /dev/tty.*` or `ls /dev/ttyUSB*`
- **Check adapter connection:** Ensure LED on ELM327 is lit

### "Failed to reset ELM327"
- Adapter may be paired but not connected (Bluetooth)
- Try unplugging and reconnecting (USB)
- Verify baud rate matches your adapter (default: 38400)

### "No data from vehicle"
- Ensure vehicle ignition is ON
- Some vehicles require engine to be running
- Adapter may need protocol selection:
  - Try different protocols in `obd_reader.c`: Change `ATSP0` to `ATSP6` (ISO 15765-4 CAN)

### Compilation Errors
- **"raylib.h not found"**: Check include path or install raylib
- **Library not found**: Ensure `libraylib.a` is in the same directory or use system raylib
- **"implicit declaration of function 'usleep'"**: Add `#include <unistd.h>` to the source file
- **"pthread.h not found"**: On some systems, install `libpthread-stubs0-dev`

### Permission Denied on Serial Port
```bash
# macOS - no special permissions usually needed

# Linux - add user to dialout group
sudo usermod -a -G dialout $USER
# Then log out and back in
```

## Technical Details

### OBD-II PIDs Supported

| PID   | Description       | Function               | Range        |
|-------|-------------------|------------------------|--------------|
| 01 0C | Engine RPM        | `OBD_ReadRPM()`        | 0-16383 RPM  |
| 01 0D | Vehicle Speed     | `OBD_ReadSpeed()`      | 0-255 km/h   |
| 01 05 | Coolant Temp      | `OBD_ReadCoolantTemp()`| -40 to 215°C |

### ELM327 Communication Protocol

The OBD reader communicates with ELM327 using AT commands over serial:

```
AT Command     Purpose
----------     -------
ATZ            Reset adapter
ATE0           Echo off
ATSP0          Auto protocol detection
010C           Read RPM (Mode 01, PID 0C)
```

### Response Format

RPM query response example:
```
Request:  01 0C\r
Response: 41 0C 1A F8 >

Parse:    41 = Response mode
          0C = PID echoed back
          1A F8 = Data bytes

Calculate: RPM = ((0x1A * 256) + 0xF8) / 4
              = (26 * 256 + 248) / 4
              = 6912 / 4
              = 1728 RPM
```

### Serial Port Configuration

- **Baud Rate:** 38400 (ELM327 default)
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Flow Control:** None

## Extending Functionality

### Adding More Gauges

You can easily add speedometer, temperature gauge, etc. by using the other OBD functions:

```c
// In your main loop
int speed = OBD_ReadSpeed(&tach.obd);
int temp = OBD_ReadCoolantTemp(&tach.obd);

// Then draw additional gauges with raylib
DrawSpeedometer(position, speed);
DrawTempGauge(position, temp);
```

### Supporting More PIDs

Edit `obd_reader.c` and add new functions following the pattern:

```c
int OBD_ReadThrottlePosition(OBDConnection* conn) {
    char response[256];
    if (!OBD_SendCommand(conn, "0111\r", response, sizeof(response))) {
        return -1;
    }
    // Parse response for PID 11 (throttle position)
    // ...
}
```

## Resources

- [OBD-II PIDs - Wikipedia](https://en.wikipedia.org/wiki/OBD-II_PIDs)
- [ELM327 Datasheet](https://www.elmelectronics.com/wp-content/uploads/2016/07/ELM327DS.pdf)
- [raylib Documentation](https://www.raylib.com)
- [Serial Programming Guide](https://www.cmrr.umn.edu/~strupp/serial.html)

## License

MIT License - Feel free to use and modify for your projects.

## Safety Warning

⚠️ **DO NOT use this application while driving.** This is for educational and diagnostic purposes only. Always follow safe driving practices and local laws.

---

**Questions or Issues?** Open an issue or contribute improvements!
