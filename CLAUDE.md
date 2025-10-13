# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**New_Workout_Plan** is a home training system using camera and accelerometer/heart rate sensors. The system uses MQTT for device communication, YOLO Pose for exercise pose detection, and Flask for AI server API.

### System Architecture

The project consists of 4 main components:

1. **AI Server** - Flask server running YOLO v11 Pose model for exercise pose detection
2. **WatchTower (Jetson Nano)** - MQTT Broker, camera streaming client, and main controller
3. **Smart Watch (ESP32)** - Heart rate sensor and display, MQTT publisher
4. **Joystick (ESP32)** - MPU6050 accelerometer sensor, MQTT publisher for motion tracking

Communication flow:
- **MQTT**: Smart Watch & Joystick → WatchTower (Broker)
- **HTTP/REST**: WatchTower ↔ AI Server (images/videos + pose analysis results)

### Current Branch Status

Current development branch: `WatchTower-joystick` (main branch: `main`)

Key branches:
- `WatchTower-joystick` - Joystick with airmouse mode (current)
- `WatchTower` - Main integrated system
- `server` - AI server development
- `jetson_mqtt` - MQTT testing

## Development Environment

### Python Environment

Python 3.10.12 virtual environment is located at `venv/`. Activate before working on Python code:

```bash
source venv/bin/activate
```

Main Python dependencies:
- **OpenCV** (`cv2`) - Camera capture and video processing
- **ultralytics** - YOLO v11 Pose model
- **Flask** - AI server REST API
- **requests** - HTTP client for WatchTower
- **paho-mqtt** - MQTT client/broker communication
- **pyserial** - UART communication with STM32
- **numpy** - Pose angle calculations

Install all dependencies:
```bash
pip install opencv-python ultralytics flask requests paho-mqtt pyserial numpy
```

### ESP32 Components (ESP-IDF)

ESP-IDF project: `mpu6050_mqtt/`

Load ESP-IDF environment:
```bash
get_idf  # Alias for '. $HOME/esp/esp-idf/export.sh'
```

Build and flash ESP32:
```bash
cd mpu6050_mqtt
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Configure ESP32 project:
```bash
cd mpu6050_mqtt
idf.py menuconfig
```

## Code Structure

### WatchTower Integrated System (`WatchTower/`)

**Complete system integrating Qt UI, MQTT broker, HTTP client, camera, and pan-tilt control:**

- `watchtower_main.py` - Main system orchestrator
  - Integrates MQTT, HTTP, camera, and UART
  - Handles Qt commands and device coordination
  - Background streaming thread for camera
- `mqtt_controller.py` - MQTT broker and device communication
  - Subscribes to Qt commands, Joystick/Watch sensor data
  - Publishes commands to devices and responses to Qt
  - Handles WatchTower protocol (start/stop/mode selection)
- `http_client.py` - AI server communication
  - REST API client for pose analysis
  - Frame transmission and result reception
- `uart_controller.py` - STM32 serial communication for pan-tilt
- `pantilt_tracker.py` - YOLO-based person tracking algorithm
- `watchtower_config.py` - Centralized configuration

**Run integrated WatchTower system:**
```bash
cd WatchTower
python watchtower_main.py
```

This system waits for Qt commands via MQTT to start workout sessions.

### MQTT Testing Tool (`WatchTower_test/`)

**CLI tool for testing MQTT communication without Qt/AI server:**

- `mqtt_test.py` - Main CLI interface for MQTT testing
- `mqtt_controller.py` - Simplified MQTT controller
- `mqtt_config.py` - MQTT-only configuration

**Test MQTT communication:**
```bash
# Start MQTT broker first
sudo systemctl start mosquitto

# Run test tool
cd WatchTower_test
python mqtt_test.py

# Commands in CLI:
# 1 - Send start command to devices
# 2 - Send stop command to devices
# 3 - Show statistics
# q - Quit
```

This tool is useful for:
- Testing Joystick and Watch MQTT connectivity
- Debugging sensor data format
- Verifying command transmission
- Monitoring real-time sensor data without full system

See [WatchTower_test/README.md](WatchTower_test/README.md) for detailed usage.

### AI Server & Streaming (`streaming/`)

**AI Server** (`streaming/ai_server/`):
- `ai_server.py` - Flask REST API server
- `pose_analyzer.py` - YOLO Pose analysis module (PoseAnalyzer class)
- `ai_config.py` - Configuration (host, port, model path, thresholds)

**WatchTower Client** (`streaming/watchtower/`):
- `watchtower_client.py` - Standalone HTTP client for streaming to AI server (legacy)
- `watchtower_config.py` - Configuration (server IP, camera settings, FPS, UART, pan-tilt)
- `uart_controller.py` - UART communication with STM32 for pan-tilt control
- `pantilt_tracker.py` - Person tracking logic based on YOLO keypoints

**Legacy TCP Streaming** (`streaming/streaming/`):
- `jetson_streaming/` - Old TCP streaming client (deprecated)
- `server_streaming/` - Old TCP streaming server (deprecated)

### MQTT Device Code (`mpu6050_mqtt/`)

ESP-IDF project for MPU6050 accelerometer with MQTT publisher:

**Key files:**
- `main/app_main.c` - Main entry point, initializes WiFi, MQTT, airmouse
- `main/mqtt_handler.c` - MQTT event handling and WatchTower protocol
- `main/sensor_task.c` - Sensor reading and data publishing
- `main/airmouse.c` - Air mouse mode implementation
- `main/config.h` - WiFi and MQTT broker configuration

**Device supports two modes:**
1. **SENSOR mode**: Publishes raw sensor data (accel_x/y/z, gyro_x/y/z)
2. **AIRMOUSE mode**: Converts sensor data to mouse movements (mouse_x/y, scroll_delta)

**WatchTower MQTT Commands** (received on `watchtower/command/joystick`):
- `{"command":"start"}` - Start sensor publishing
- `{"command":"stop"}` - Stop sensor publishing
- `{"command":"airmouse_mode"}` - Switch to airmouse mode
- `{"command":"sensor_mode"}` - Switch to sensor mode
- `{"command":"calibrate"}` - Calibrate airmouse

**Published Topics:**
- `joystick/sensor/data` - Sensor or airmouse data (JSON)
- `joystick/status` - Device status (ready/stopped/calibrated)

**Configuration:**
Edit `main/config.h` to set WiFi credentials and MQTT broker IP:
```c
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-password"
#define MQTT_BROKER_URL "mqtt://192.168.1.100:1883"
```

### YOLO Pose Detection (`yolo_test/`)

**`webcam_yolo.py`** - Standalone webcam test for YOLO v11 Pose with T-pose detection:
- Loads `yolo11s-pose.pt` model (YOLO v11 Pose small variant)
- Detects 17 keypoints (COCO format): nose, eyes, ears, shoulders, elbows, wrists, hips, knees, ankles
- Implements T-pose validation using angle calculations:
  - `calculate_angle(p1, p2, p3)` - Calculates angle at p2 using 3 points
  - `calculate_horizontal_angle(p1, p2)` - Calculates deviation from horizontal
  - `check_t_pose()` - Validates T-pose with scoring (0-100%) and feedback
- T-pose criteria: arms straight (>160°), arms horizontal (<20° deviation)

### Utility Tools

**`mqtt_monitor.py`** - Monitor all MQTT topics in real-time:
```bash
python mqtt_monitor.py
```
Subscribes to all topics (`#` wildcard) and displays incoming messages with timestamps.

**`jeston_mqtt/`** - Simple MQTT sender/receiver for testing:
- `sender.py` - Publish test messages
- `receiver.py` - Subscribe and display messages
- `start_mosquitto.sh` - Start MQTT broker script

## Common Commands

### Run AI Server

Start AI server on PC or server:
```bash
cd streaming/ai_server
python ai_server.py
```

Default: `http://0.0.0.0:5000`

### Run WatchTower System

**Option 1: Full integrated system (waits for Qt)**
```bash
cd WatchTower
python watchtower_main.py
```

**Option 2: Standalone streaming client (legacy)**
```bash
cd streaming/watchtower
python watchtower_client.py
```

**Important**: Update AI server IP in config files:
- `WatchTower/watchtower_config.py`
- `streaming/watchtower/watchtower_config.py`

```python
AI_SERVER_HOST = '192.168.1.100'  # Replace with actual AI server IP
```

### Test MQTT Communication

**Start MQTT broker:**
```bash
sudo systemctl start mosquitto

# Check status
sudo systemctl status mosquitto

# Check port
sudo lsof -i :1883
```

**Run MQTT test tool:**
```bash
cd WatchTower_test
python mqtt_test.py
```

**Monitor all MQTT traffic:**
```bash
python mqtt_monitor.py
```

### Run YOLO Test

Test YOLO pose detection with webcam:
```bash
python yolo_test/webcam_yolo.py
```

### ESP-IDF Development

Build and flash ESP32 joystick:
```bash
cd mpu6050_mqtt
get_idf
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Configure WiFi/MQTT:
```bash
cd mpu6050_mqtt
get_idf
idf.py menuconfig
# Navigate to: Component config -> ESP32 Configuration
```

Clean build artifacts:
```bash
cd mpu6050_mqtt
idf.py fullclean
```

Monitor serial output:
```bash
idf.py -p /dev/ttyUSB0 monitor
```

## Key Implementation Details

### MQTT Protocol (WatchTower)

**Device → WatchTower Topics:**
- `joystick/sensor/data` - Joystick sensor or airmouse data
- `watch/sensor/heartrate` - Watch heart rate data
- `joystick/status` - Joystick status messages
- `watch/status` - Watch status messages

**WatchTower → Device Topics:**
- `watchtower/command/joystick` - Commands to joystick
- `watchtower/command/watch` - Commands to watch

**Qt ↔ WatchTower Topics:**
- `qt/command/select_mode` - Qt selects workout mode
- `qt/command/start` - Qt starts workout
- `qt/command/stop` - Qt stops workout
- `qt/response/mode_selected` - WatchTower confirms mode
- `qt/response/analysis` - WatchTower sends AI analysis results
- `qt/response/status` - WatchTower status updates

**Message Formats:**

Joystick sensor data:
```json
{
  "accel_x": 0.52,
  "accel_y": -0.31,
  "accel_z": 9.78,
  "gyro_x": 0.01,
  "gyro_y": -0.02,
  "gyro_z": 0.00,
  "timestamp": 1234567890
}
```

Joystick airmouse data:
```json
{
  "mode": "airmouse",
  "mouse_x": 10.5,
  "mouse_y": -5.2,
  "scroll_delta": 1,
  "timestamp": 1234567890
}
```

WatchTower commands:
```json
{
  "command": "start",
  "mode": "t_pose",
  "timestamp": 1234567890
}
```

### YOLO Pose Keypoint Indices

When working with pose detection, keypoint indices (0-16):
- 0: Nose
- 1-2: Eyes (left, right)
- 3-4: Ears (left, right)
- 5-6: Shoulders (left, right)
- 7-8: Elbows (left, right)
- 9-10: Wrists (left, right)
- 11-12: Hips (left, right)
- 13-14: Knees (left, right)
- 15-16: Ankles (left, right)

Access keypoints with confidence filtering (threshold 0.5):
```python
keypoints = results[0].keypoints[0]
xy = keypoints.xy.cpu().numpy()[0]  # (17, 2) coordinates
conf = keypoints.conf.cpu().numpy()[0]  # (17,) confidence
left_shoulder = xy[5] if conf[5] > 0.5 else None
```

### HTTP REST API Protocol

**AI Server Endpoints:**
- `GET /api/health` - Health check
- `GET /api/status` - Server status
- `POST /api/mode/select` - Select workout mode
- `POST /api/stream/frame` - Send frame for analysis
- `POST /api/stream/stop` - Stop streaming session

Frame transmission (POST /api/stream/frame):
```json
{
  "frame": "base64_encoded_jpeg_image",
  "timestamp": 1234567890
}
```

Analysis response:
```json
{
  "status": "success",
  "is_correct": false,
  "score": 75,
  "feedback": "왼팔 수평 (25°)",
  "keypoints": {
    "left_arm_angle": 165.3,
    "right_arm_angle": 170.2,
    "left_horizontal": 25.1,
    "right_horizontal": 15.3
  },
  "tracking": {
    "center_x": 320,
    "center_y": 240,
    "bbox": [100, 50, 540, 430]
  }
}
```

### Pan-Tilt Camera Control (UART Protocol)

WatchTower communicates with STM32 via UART to control MG996R servo motors.

**Command format**: `<CMD>:<VALUE>\n`

Supported commands (MG996R: -60~60° range, center at 0,0):
- `PAN:0\n` - Set Pan angle to 0° (-60~60°, center)
- `TILT:-30\n` - Set Tilt angle to -30° (-60~60°)
- `PANTILT:0,0\n` - Set both Pan and Tilt simultaneously
- `CENTER\n` - Return to center position (0,0)
- `STOP\n` - Stop servo motors

**Tracking algorithm:**
1. Extract bounding box from YOLO detection
2. Calculate center point of detected person
3. Compare with frame center, calculate error
4. Apply proportional control to compute Pan/Tilt angles
5. Apply smoothing filter (moving average over 3 frames)
6. Send angle commands via UART to STM32

**Key modules:**
- `uart_controller.py` - Serial communication (pySerial)
- `pantilt_tracker.py` - Tracking algorithm with smoothing

### Configuration Management

Always use config files for server addresses and hardware settings:

**AI Server:**
- `streaming/ai_server/ai_config.py` - Host, port, model path, thresholds

**WatchTower:**
- `WatchTower/watchtower_config.py` - Full system config (MQTT, HTTP, Camera, UART)
- `streaming/watchtower/watchtower_config.py` - Legacy streaming client config

**MQTT Test:**
- `WatchTower_test/mqtt_config.py` - MQTT broker settings and topics

**ESP32:**
- `mpu6050_mqtt/main/config.h` - WiFi credentials and MQTT broker URL

When deploying to Jetson Nano:
- Update `AI_SERVER_HOST` to AI server IP
- Update `UART_PORT` to actual STM32 port (`/dev/ttyUSB0` or `/dev/ttyACM0`)
- Set UART permissions: `sudo usermod -a -G dialout $USER` (requires re-login)

### Adding New Exercise Modes

1. Add mode to `streaming/ai_server/ai_config.py`:
```python
SUPPORTED_MODES = ['t_pose', 'squat', 'pushup', 'new_mode']
```

2. Implement analysis method in `streaming/ai_server/pose_analyzer.py`:
```python
def _analyze_new_mode(self, xy, conf):
    # Keypoint analysis logic
    return {
        'status': 'success',
        'is_correct': True/False,
        'score': 0-100,
        'feedback': 'feedback message',
        'keypoints': {...}
    }
```

3. Connect mode in `PoseAnalyzer.analyze_frame()`:
```python
if self.current_mode == 'new_mode':
    return self._analyze_new_mode(xy, conf)
```

## Hardware Components

- **Jetson Nano** - WatchTower main controller
- **STM32F411R** - Servo motor control via UART
- **ESP32** (x2) - Watch & Joystick devices
- **MG996R servo motors** (x2) - Pan-tilt camera mount
- **MPU6050** - Accelerometer/gyroscope (Joystick)
- **Heart rate sensor** - Pulse measurement (Watch)
- **USB Camera** - Exercise recording

## Git Workflow

**Key branches:**
- `main` - Main stable branch
- `WatchTower-joystick` - Current development (joystick with airmouse)
- `WatchTower` - Integrated system development
- `server` - AI server development
- `jetson_mqtt` - MQTT testing branch

**When merging branches with conflicts:**
- ESP32 code conflicts: Keep airmouse functionality from `WatchTower-joystick`
- Python code: Integrate new features while preserving existing functionality
- Config files: Merge settings, prioritize hardware-specific configurations

## Notes

- The project uses Korean language for documentation and UI messages
- Python virtual environment must be activated for all Python work
- ESP-IDF environment must be loaded for ESP32 development
- MQTT broker (mosquitto) must be running on WatchTower (Jetson Nano)
- AI server and WatchTower can run on different machines (configure IPs)
- UART permissions required for pan-tilt control on Jetson Nano

See [ToDo.md](ToDo.md) for current development priorities and [README.md](README.md) for system architecture diagrams.
