# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32 IoT project using ESP-IDF framework to drive a 240x280 ST7789 LCD display with CST816S capacitive touch controller, integrated with LVGL for GUI rendering and WiFi connectivity with NTP time synchronization.

**Target**: ESP32 (2MB flash)
**Framework**: ESP-IDF (Espressif IoT Development Framework)
**Key Features**: LCD touchscreen, WiFi connectivity, NTP clock synchronization

## Build Commands

```bash
# Configure project (opens menuconfig)
idf.py menuconfig

# Build the project
idf.py build

# Flash to device (make sure device is connected)
idf.py flash

# Monitor serial output
idf.py monitor

# Flash and immediately monitor
idf.py flash monitor

# Clean build artifacts
idf.py fullclean

# Reconfigure from scratch
idf.py reconfigure
```

## Architecture

### Application Structure

The main application is organized into modular subsystems in `main/`:

- **[main.c](main/main.c)** - Application entry point
  - Orchestrates initialization sequence: hardware → WiFi → UI
  - Minimal orchestration logic; delegates to subsystem modules

- **[hardware.c/h](main/hardware.c)** - Hardware abstraction layer
  - Initializes ST7789 LCD (SPI) and CST816S touch (I2C)
  - Exposes global hardware objects (`lcd`, `touch`)
  - Pin configuration defined in [hardware.h](main/hardware.h:14-27)

- **[ui.c/h](main/ui.c)** - LVGL GUI management
  - Initializes LVGL with display/input drivers
  - Creates multi-screen UI: main clock screen, WiFi selection, password input
  - Runs LVGL event loop in dedicated FreeRTOS task
  - Handles WiFi status callbacks and updates UI accordingly

- **[wifi.c/h](main/wifi.c)** - WiFi connectivity
  - WiFi initialization, scanning, and connection management
  - Asynchronous connection with event-driven callbacks
  - NTP time synchronization (pool.ntp.org, time.google.com)
  - Timezone: KST-9 (Korean Standard Time)
  - **NVS-based credential persistence**: Automatically saves WiFi credentials on successful connection
  - Auto-reconnect on boot using saved credentials via `wifi_auto_connect()`

### Component Structure

Low-level drivers reside in `components/`:

- **components/st7789/** - ST7789 LCD driver
  - SPI-based display driver (240x280 RGB565)
  - Key API: `st7789_init()`, `st7789_set_window()`, `st7789_write_data()`

- **components/cst816s/** - CST816S touch controller driver
  - I2C-based capacitive touch driver
  - Key API: `cst816s_init()`, `cst816s_read()`

- **components/lvgl/** - LVGL graphics library (v8.x)
  - Configured via [components/lv_conf.h](components/lv_conf.h)
  - Includes widget demos enabled in CMakeLists.txt

### Hardware Pin Configuration

Defined in [main/hardware.h](main/hardware.h:14-27):
- **LCD (SPI)**: MOSI=23, CLK=18, CS=5, DC=25, RST=4, BL=26
- **Touch (I2C)**: SDA=21, SCL=22, RST=16, IRQ=17

### Key Integration Patterns

**LVGL ↔ Hardware Integration** ([ui.c](main/ui.c:92-119))
- `disp_flush()`: Bridges LVGL rendering to ST7789 via `st7789_set_window()` and `st7789_write_data()`
- `touchpad_read()`: Polls CST816S via `cst816s_read()` for touch coordinates
- Double-buffered DMA memory (1/4 screen size each buffer)
- ESP timer calls `lv_tick_inc()` every 2ms
- FreeRTOS task calls `lv_timer_handler()` every 5ms in `lvgl_task()`

**WiFi ↔ UI Integration** ([ui.c](main/ui.c:172-179), [wifi.c](main/wifi.c))
- WiFi connection is asynchronous with callback-based status notification
- UI registers `wifi_status_callback()` which sets flags checked by `lvgl_task()`
- Background color feedback: green for success, red for failure (2-second flash)
- WiFi event handler in `wifi.c` triggers SNTP sync and NVS credential save on successful connection

**WiFi Credential Persistence** ([wifi.c](main/wifi.c:40-153), [main.c](main/main.c:51-60))
- On successful connection, WiFi credentials are automatically saved to NVS flash storage
- On boot, `main.c` calls `wifi_auto_connect()` to attempt reconnection with saved credentials
- Credentials persist across power cycles (survives ESP32 restarts)
- Use `wifi_clear_saved_credentials()` to erase stored credentials
- NVS namespace: `"wifi_config"`, keys: `"ssid"`, `"password"`

**UI Flow**
1. Boot: Auto-connect attempt with saved credentials (if available)
2. Main screen: Clock display with WiFi/Right buttons
3. WiFi button → WiFi scan screen → Network list
4. Select network → Password screen (if secured) → Connection attempt
5. Async callback updates main screen background color based on result
6. Successful connection auto-saves credentials for next boot

### CMake Dependencies

[main/CMakeLists.txt](main/CMakeLists.txt) requires:
```cmake
REQUIRES lvgl st7789 cst816s driver esp_timer esp_wifi esp_event esp_netif nvs_flash
```

Each component registers via `idf_component_register()`:
- `SRCS`: Source files
- `INCLUDE_DIRS`: Public headers
- `REQUIRES`: Component dependencies

### LVGL Configuration

Modify [components/lv_conf.h](components/lv_conf.h) for LVGL features:
- Memory allocation settings
- Color depth (RGB565)
- Font enablement (montserrat_28, montserrat_40 used in clock UI)
- Widget demos (enabled: widgets, benchmark, stress)

### Serial Port Configuration

If device not found during flash:
```bash
idf.py menuconfig
# Navigate to: Serial flasher config -> Flash port
```

Or specify directly:
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```
