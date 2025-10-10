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
- **numpy** - Pose angle calculations

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

### AI Server & WatchTower (`streaming/`)

**AI Server** (`streaming/ai_server/`):
- `ai_server.py` - Flask REST API server
- `pose_analyzer.py` - YOLO Pose analysis module (PoseAnalyzer class)
- `ai_config.py` - Configuration (host, port, model path, thresholds)

**WatchTower Client** (`streaming/watchtower/`):
- `watchtower_client.py` - HTTP client for streaming to AI server
- `watchtower_config.py` - Configuration (server IP, camera settings, FPS, UART, pan-tilt)
- `uart_controller.py` - UART communication with STM32 for pan-tilt control
- `pantilt_tracker.py` - Person tracking logic based on YOLO keypoints

**Legacy TCP Streaming** (`streaming/streaming/`):
- `jetson_streaming/` - Old TCP streaming client (deprecated)
- `server_streaming/` - Old TCP streaming server (deprecated)

Key protocol details (HTTP/REST):
- `POST /api/mode/select` - Select workout mode (t_pose, squat, pushup)
- `POST /api/stream/frame` - Send base64-encoded JPEG frame, receive analysis result
- `POST /api/stream/stop` - Stop streaming session
- `GET /api/health` - Health check
- `GET /api/status` - Server status

### YOLO Pose Detection (`yolo_test/`)

**`webcam_yolo.py`** - Standalone webcam test for YOLO v11 Pose with T-pose detection:
- Loads `yolo11s-pose.pt` model (YOLO v11 Pose small variant)
- Detects 17 keypoints (COCO format): nose, eyes, ears, shoulders, elbows, wrists, hips, knees, ankles
- Implements T-pose validation using angle calculations:
  - `calculate_angle(p1, p2, p3)` - Calculates angle at p2 using 3 points
  - `calculate_horizontal_angle(p1, p2)` - Calculates deviation from horizontal
  - `check_t_pose()` - Validates T-pose with scoring (0-100%) and feedback
- T-pose criteria: arms straight (>160°), arms horizontal (<20° deviation)

### MQTT Device Code (`mpu6050_mqtt/`)

ESP-IDF project for MPU6050 accelerometer with MQTT publisher:
- Built using ESP-IDF CMake system
- Configuration in `sdkconfig` (run `idf.py menuconfig` to modify)
- Publishes sensor data to WatchTower MQTT broker

## Common Commands

### Run AI Server and WatchTower

Run AI server (on PC/server):
```bash
cd streaming/ai_server
python ai_server.py
```

Run WatchTower client (on Jetson Nano or development PC):
```bash
cd streaming/watchtower
python watchtower_client.py
```

**Important**: Update `streaming/watchtower/watchtower_config.py` with AI server IP:
```python
AI_SERVER_HOST = '192.168.1.100'  # Replace with actual AI server IP
```

### Run YOLO Test

Test YOLO pose detection with webcam:
```bash
python yolo_test/webcam_yolo.py
```

### ESP-IDF Development

Build and flash ESP32:
```bash
cd mpu6050_mqtt
get_idf
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Configure ESP32 project:
```bash
cd mpu6050_mqtt
get_idf
idf.py menuconfig
```

Clean build artifacts:
```bash
cd mpu6050_mqtt
idf.py fullclean
```

## Key Implementation Details

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

Frame transmission format (POST /api/stream/frame):
```json
{
  "frame": "base64_encoded_jpeg_image",
  "timestamp": 1234567890
}
```

Response (analysis result):
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

WatchTower communicates with STM32 via UART to control MG966R servo motors.

**Command format**: `<CMD>:<VALUE>\n`

Supported commands (MG996R: 0-120° range):
- `PAN:60\n` - Set Pan angle to 60° (0-120°, center)
- `TILT:45\n` - Set Tilt angle to 45° (0-120°)
- `PANTILT:60,60\n` - Set both Pan and Tilt simultaneously
- `CENTER\n` - Return to center position (60,60)
- `STOP\n` - Stop servo motors

**Tracking algorithm**:
1. Extract bounding box from YOLO detection
2. Calculate center point of detected person
3. Compare with frame center, calculate error
4. Apply proportional control to compute Pan/Tilt angles
5. Apply smoothing filter (moving average over 3 frames)
6. Send angle commands via UART to STM32

**Key modules**:
- `uart_controller.py` - Serial communication (pySerial)
- `pantilt_tracker.py` - Tracking algorithm with smoothing

### Configuration Management

Always use config files for server addresses and hardware settings:
- `streaming/ai_server/ai_config.py` - AI server host, port, model path, thresholds
- `streaming/watchtower/watchtower_config.py` - AI server IP, camera settings, FPS, UART port, pan-tilt settings
- When deploying to Jetson Nano:
  - Update `WatchTowerConfig.AI_SERVER_HOST` to AI server IP
  - Update `WatchTowerConfig.UART_PORT` to actual STM32 port (`/dev/ttyUSB0` or `/dev/ttyACM0`)
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

## Project Status

Current development branch: `server` (main branch: `main`)

Hardware components:
- Jetson Nano (WatchTower)
- STM32F411R (servo motor control)
- ESP32 (Watch & Joystick)
- MG966R servo motors (pan-tilt camera)
- MPU6050 accelerometer (Joystick)
- Heart rate sensor (Watch)

The project uses Korean language for documentation and UI messages.

See [ToDo.md](ToDo.md) for current development priorities and [README.md](README.md) for detailed system architecture diagrams.
