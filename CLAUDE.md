# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **home workout training system** integrating multiple embedded devices and AI-powered pose analysis. The system comprises:

- **Qt Application** (Qt 5.12+ with C++17): Desktop UI for workout selection, real-time feedback, and air mouse control
- **AI Server** (Flask + YOLOv11 Pose): Analyzes exercise form and provides real-time feedback
- **WatchTower** (Python): Orchestration layer coordinating MQTT, HTTP, camera streaming, and pan-tilt tracking
- **ESP32 Joystick** (ESP-IDF): MPU6050-based air mouse and sensor data publisher
- **Smart Watch** (ESP32 + LVGL): ST7789 LCD touchscreen interface with WiFi connectivity

**Communication Architecture**: All components communicate via MQTT (Mosquitto broker on Jetson Nano), with AI analysis via HTTP/REST.

## System Architecture

### High-Level Data Flow

```
┌─────────────┐      MQTT       ┌──────────────┐      MQTT      ┌────────────┐
│  Qt App     │ ←──────────────→ │  WatchTower  │ ←────────────→ │  Joystick  │
│ (Desktop UI)│                  │   (Jetson)   │                │  (ESP32)   │
└─────────────┘                  └──────────────┘                └────────────┘
                                        │
                                        │ HTTP
                                        ↓
                                 ┌──────────────┐
                                 │  AI Server   │
                                 │ (Flask+YOLO) │
                                 └──────────────┘
                                        │
                                        │ UART
                                        ↓
                                 ┌──────────────┐
                                 │  Pan-Tilt    │
                                 │   (STM32)    │
                                 └──────────────┘
```

### Communication Protocols

**MQTT Topics** (Broker: Jetson Nano at 10.10.16.111:1883):
- `qt/command/*` - Qt → WatchTower commands (select_mode, start, stop, pose_index, request_analysis)
- `qt/response/*` - WatchTower → Qt responses (mode_selected, analysis, frame, error, status)
- `watchtower/command/*` - WatchTower → Devices commands (joystick, watch)
- `joystick/sensor/data` - Joystick → Qt (air mouse or sensor data)
- `watch/sensor/heartrate` - Watch → Qt (heart rate data)

**HTTP API** (AI Server at 192.168.1.100:5000):
- `POST /api/mode/select` - Select exercise mode
- `POST /api/stream/frame` - Analyze single frame
- `POST /api/stream/stop` - Stop streaming analysis
- `GET /api/status` - Server status
- `GET /api/health` - Health check

### Exercise Modes & Pose Sequences

The AI server supports 17 exercises organized into 3 routines + 14 individual exercises:

**Routines**:
- `bodyweight_routine`: squat, pushup, plank, lunge (4 poses)
- `kettlebell_routine`: kettlebell_swing, kettlebell_deadlift, side_lunge, bridge, knee_drive (5 poses)
- `barbell_routine`: barbell_row, barbell_upright_row, barbell_overhead_press, barbell_biceps_curl, barbell_reverse_curl (5 poses)

**Individual Exercises**: squat, pushup, plank, lunge, kettlebell_swing, kettlebell_deadlift, side_lunge, bridge, knee_drive, barbell_row, barbell_upright_row, barbell_overhead_press, barbell_biceps_curl, barbell_reverse_curl

Each exercise has multiple pose checkpoints (e.g., squat has "stand" and "down"). Qt cycles through poses using `pose_index`.

## Build & Run Commands

### Qt Application (Desktop UI)

**Location**: `Qt_app_ver2/`

**Requirements**: Qt 5.12+, Qt MQTT module, C++17 compiler

```bash
cd Qt_app_ver2

# Build with qmake
qmake workout_app.pro
make

# Run
./workout_app

# Or use Qt Creator
# 1. Open workout_app.pro in Qt Creator
# 2. Build and run (Ctrl+R)
```

**Configuration**: Edit `config.json` to set MQTT broker address, topics, and UI settings.

### AI Server (Flask)

**Location**: `ai_server_v1/`

**Requirements**: Python 3.x, Flask, OpenCV, NumPy, YOLOv11 Pose model

```bash
cd ai_server_v1

# Install dependencies (first time)
pip install flask opencv-python numpy ultralytics

# Run server
python ai_server.py

# Server runs at http://0.0.0.0:5000
```

**Model**: Uses `yolo11s-pose.pt` for pose detection (17 COCO keypoints).

### WatchTower (Orchestration Layer)

**Location**: `WatchTower/`

**Requirements**: Python 3.x, paho-mqtt, requests, OpenCV, pyserial

```bash
cd WatchTower

# Install dependencies
pip install paho-mqtt requests opencv-python pyserial

# Configure settings
# Edit watchtower_config.py to set:
#   - AI_SERVER_HOST (AI server IP)
#   - MQTT_BROKER_HOST (Jetson IP)
#   - CAMERA_ID (USB camera device ID)
#   - UART_PORT (Pan-tilt STM32 serial port)

# Run WatchTower
python watchtower_main.py
```

**Responsibilities**:
- Receive Qt commands via MQTT
- Forward mode selection to AI server via HTTP
- Stream camera frames to AI server
- Publish analysis results back to Qt
- Control pan-tilt tracking via UART
- Relay commands to joystick/watch

### ESP32 Joystick

**Location**: `joysitck/` (note: typo in original directory name)

**Framework**: ESP-IDF

```bash
cd joysitck

# Configure WiFi and MQTT broker
# Edit main/config.h:
#   - WIFI_SSID, WIFI_PASSWORD
#   - MQTT_BROKER_URL (Jetson IP)

# Build
idf.py build

# Flash and monitor
idf.py flash monitor

# Clean build
idf.py fullclean
```

**Modes**:
- **Air Mouse Mode**: Publishes MPU6050 gyro data as mouse movements to `joystick/sensor/data`
- **Sensor Mode**: Publishes raw sensor readings (triggered during workout)

**Mode Switching**: WatchTower sends commands via `watchtower/command/joystick` with `{"command": "airmouse_mode"}` or `{"command": "sensor_mode"}`.

### Smart Watch (ESP32 + LVGL)

**Location**: `smart_watch/`

**Framework**: ESP-IDF with LVGL v8.x

```bash
cd smart_watch

# Build
idf.py build

# Flash and monitor
idf.py flash monitor

# Configure project
idf.py menuconfig
```

**Hardware**: ST7789 LCD (240x280, SPI), CST816S touch (I2C), ESP32 (2MB flash)

**Key Features**: WiFi connectivity, NTP time sync, touch UI, auto-reconnect with saved credentials in NVS.

## Development Workflow

### Starting a Workout Session

1. **Start MQTT Broker** (Jetson):
   ```bash
   sudo systemctl start mosquitto
   # Verify: sudo systemctl status mosquitto
   ```

2. **Start AI Server**:
   ```bash
   cd ai_server_v1
   python ai_server.py
   ```

3. **Start WatchTower**:
   ```bash
   cd WatchTower
   python watchtower_main.py
   ```

4. **Run Qt App**:
   ```bash
   cd Qt_app_ver2
   ./workout_app
   ```

5. **Workflow**:
   - Qt: Select exercise → WatchTower: Forward to AI server → AI: Initialize pose analyzer
   - Qt: Click "Start" → WatchTower: Switch joystick to sensor mode, start camera streaming
   - WatchTower: Continuously capture frames, send to AI server, publish results to Qt
   - Qt: Display real-time feedback (score, feedback text, video feed)
   - User: Perform exercise poses in sequence
   - Qt: Send pose_index updates → WatchTower: Update AI server pose index → AI: Analyze next pose
   - Qt: Click "Stop" → WatchTower: Stop camera, switch joystick to air mouse mode

### Testing Individual Components

**MQTT Message Monitoring**:
```bash
# Subscribe to all topics
mosquitto_sub -h localhost -t '#' -v

# Qt commands only
mosquitto_sub -h localhost -t 'qt/command/#' -v

# WatchTower responses
mosquitto_sub -h localhost -t 'qt/response/#' -v

# Joystick data
mosquitto_sub -h localhost -t 'joystick/sensor/data' -v
```

**Manual MQTT Commands**:
```bash
# Select exercise mode
mosquitto_pub -h localhost -t 'qt/command/select_mode' -m '{"mode":"squat","timestamp":1699999999}'

# Start workout
mosquitto_pub -h localhost -t 'qt/command/start' -m '{"command":"start","mode":"squat","timestamp":1699999999}'

# Stop workout
mosquitto_pub -h localhost -t 'qt/command/stop' -m '{"command":"stop","timestamp":1699999999}'

# Switch joystick to air mouse mode
mosquitto_pub -h localhost -t 'watchtower/command/joystick' -m '{"command":"airmouse_mode"}'
```

**AI Server Testing**:
```bash
# Check health
curl http://192.168.1.100:5000/api/health

# Select mode
curl -X POST http://192.168.1.100:5000/api/mode/select \
  -H "Content-Type: application/json" \
  -d '{"mode":"squat"}'

# Check status
curl http://192.168.1.100:5000/api/status
```

## Key Implementation Details

### Qt Application Structure

**Page Widgets** (`Qt_app_ver2/`):
- `main_menu_page_widget` - Main menu (start workout, settings)
- `exercise_selection_page_widget` - Exercise catalog with categories (Bodyweight, Kettlebell, Barbell)
- `workout_page_widget` - Real-time workout display (video feed, score, feedback, pose sequence)
- `settings_page_widget` - MQTT connection, air mouse sensitivity, calibration
- `result_page_widget` - Post-workout results

**Key Classes**:
- `MainWindow` - Page navigation orchestration, MQTT message routing
- `AirMouseManager` - Processes joystick data → cursor movements with smoothing/sensitivity
- `CursorCanvas` - Renders cursor overlay on workout video
- `VideoFrameWidget` - Displays base64-encoded JPEG frames from WatchTower
- `Config` - Manages config.json persistence (MQTT settings, UI preferences)
- `ExerciseCatalog` - Exercise database with Korean names → English mode mapping

**Exercise Name Mapping** (Korean UI → MQTT):
```cpp
QMap<QString, QString> exerciseMap = {
    {"스쿼트", "squat"},
    {"푸쉬업", "pushup"},
    {"플랭크", "plank"},
    // ... etc
};
```

### WatchTower Integration Patterns

**Asynchronous Workflow**:
1. Qt command received → MQTT callback triggers handler
2. Handler sends HTTP request to AI server (synchronous, blocking)
3. Handler publishes MQTT response back to Qt
4. Camera streaming runs in background thread (`_streaming_loop`)
5. Each frame: Capture → Encode JPEG → Base64 → HTTP POST to AI → Publish result to Qt

**Pan-Tilt Tracking**:
- Enabled after first successful pose analysis
- Uses keypoint bounding box center as tracking target
- Smooth trajectory with dead zone to avoid jitter
- UART protocol: `PAN=<angle>;TILT=<angle>\n` to STM32

### AI Server Pose Analysis

**Keypoint Detection**: YOLOv11 Pose extracts 17 COCO keypoints (nose, eyes, ears, shoulders, elbows, wrists, hips, knees, ankles).

**Pose Validation** (`ai_server_v1/pose_analyzer.py`):
1. Check keypoint confidence (threshold: 0.5)
2. Calculate joint angles using `_calculate_angle(p1, p2, p3)`
3. Compare against thresholds in `ai_config.py` (e.g., squat knee angle 70-110°)
4. Compute score (0-100) based on how many criteria are met
5. Generate feedback message for incorrect posture

**Camera Orientation**:
- **Side view**: All exercises except knee_drive, side_lunge
- **Front view**: knee_drive, side_lunge

### ESP32 Joystick Air Mouse

**Data Format** (published to `joystick/sensor/data`):
```json
{
  "mode": "airmouse",
  "mouse_x": 10.5,     // Gyro X delta
  "mouse_y": -5.2,     // Gyro Y delta
  "scroll_delta": 0,
  "button_pressed": false,
  "timestamp": 1234567890
}
```

**Qt Processing**:
- Apply sensitivity multiplier (configurable in settings)
- Smooth with moving average (default 3 frames)
- Clamp to screen bounds
- Display cursor overlay on workout video or test canvas

### Configuration Files

**WatchTower** (`WatchTower/watchtower_config.py`):
- AI_SERVER_HOST, AI_SERVER_PORT
- MQTT_BROKER_HOST, MQTT_BROKER_PORT
- CAMERA_ID, CAMERA_WIDTH, CAMERA_HEIGHT, STREAM_FPS
- UART_PORT, UART_BAUDRATE (pan-tilt control)
- SUPPORTED_MODES (must match AI server)

**AI Server** (`ai_server_v1/ai_config.py`):
- SUPPORTED_MODES (list of all exercises)
- MODE_POSES (dict mapping mode → list of pose checkpoints)
- Pose-specific thresholds (e.g., SQUAT_KNEE_ANGLE_DOWN_MIN)

**Qt App** (`Qt_app_ver2/config.json`):
- mqtt_broker (host, port, client_id)
- mqtt_topics (all topic names)
- ui_settings (window size, auto_connect)
- exercise_modes (displayed in exercise selection)

**ESP32 Joystick** (`joysitck/main/config.h`):
- WIFI_SSID, WIFI_PASSWORD
- MQTT_BROKER_URL (mqtt://<jetson-ip>:1883)
- DEFAULT_PUBLISH_INTERVAL_MS

## Troubleshooting

### MQTT Connection Issues

**Qt App cannot connect to broker**:
- Verify Mosquitto is running: `sudo systemctl status mosquitto`
- Check firewall: `sudo ufw allow 1883`
- Verify broker IP in `config.json` matches Jetson IP
- Test with `mosquitto_sub -h <broker-ip> -t '#' -v`

**Joystick not publishing data**:
- Check WiFi connection (same network as Jetson)
- Verify MQTT_BROKER_URL in `joysitck/main/config.h`
- Monitor serial output: `idf.py monitor`
- Test broker connectivity: `mosquitto_pub -h <broker-ip> -t test -m hello`

### AI Server Issues

**"AI server connection failed"**:
- Verify server is running: `curl http://<ai-server-ip>:5000/api/health`
- Check AI_SERVER_HOST in `watchtower_config.py`
- Ensure Flask app bound to `0.0.0.0` (not `127.0.0.1`)
- Check firewall on AI server machine

**Low pose detection accuracy**:
- Ensure adequate lighting
- Verify camera orientation (side vs front view)
- Check exercise is in SUPPORTED_MODES
- Review keypoint confidence in AI server logs
- Adjust thresholds in `ai_config.py` if needed

### Camera & Pan-Tilt Issues

**"Camera initialization failed"**:
- Check CAMERA_ID (usually 0 for /dev/video0)
- Verify camera permissions: `ls -l /dev/video*`
- Test with: `v4l2-ctl --list-devices`
- Try different camera: `idf.py menuconfig` → change CAMERA_ID

**Pan-tilt not tracking**:
- Verify UART_PORT in `watchtower_config.py` (e.g., /dev/ttyUSB0)
- Check UART permissions: `sudo usermod -a -G dialout $USER`
- Test UART: `python WatchTower/pantilt_test.py`
- Set PANTILT_VERBOSE=True for detailed logs

### ESP32 Build Errors

**ESP-IDF not found**:
```bash
# Source ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Or add to ~/.bashrc:
alias get_idf='. $HOME/esp/esp-idf/export.sh'
```

**Serial port permission denied**:
```bash
sudo usermod -a -G dialout $USER
# Log out and back in for group change to take effect
```

## Important Notes

- **MQTT QoS**: Commands use QoS 1 (at least once), sensor data uses QoS 0 (at most once) for performance
- **Air Mouse Mode Switching**: Joystick automatically switches to sensor mode during workout, reverts to air mouse on stop
- **Pose Index Synchronization**: Qt sends `pose_index` updates before requesting analysis to ensure AI server validates correct pose
- **Frame Encoding**: All video frames are JPEG-compressed then Base64-encoded for MQTT/HTTP transport
- **WiFi Credentials**: Smart watch auto-saves WiFi credentials to NVS on successful connection, auto-reconnects on boot
- **Thread Safety**: WatchTower camera streaming runs in background thread; MQTT callbacks execute in MQTT loop thread
- **Exercise Names**: UI displays Korean names, but MQTT/HTTP use English mode identifiers (e.g., "스쿼트" → "squat")

## Common Modifications

### Adding a New Exercise

1. **AI Server** (`ai_server_v1/ai_config.py`):
   - Add to `SUPPORTED_MODES` list
   - Define pose sequence in `MODE_POSES` dict
   - Add threshold constants (e.g., `NEW_EXERCISE_ANGLE_MIN`)

2. **AI Server** (`ai_server_v1/pose_analyzer.py`):
   - Implement `_analyze_<exercise>_<pose>()` methods
   - Add to `_analyze_pose()` dispatcher

3. **WatchTower** (`watchtower_config.py`):
   - Add to `SUPPORTED_MODES`

4. **Qt App** (`Qt_app_ver2/exercise_catalog.cpp`):
   - Add to `exercisesData` with Korean name, English mode, category
   - Update `exerciseMap` for name mapping

5. **Qt App** (`Qt_app_ver2/config.json`):
   - Add to `exercise_modes` (optional, for quick access)

### Changing MQTT Broker IP

1. **Jetson**: Update Mosquitto config `/etc/mosquitto/mosquitto.conf` (or use default 0.0.0.0)
2. **Qt App**: Edit `config.json` → `mqtt_broker.host`
3. **WatchTower**: Edit `watchtower_config.py` → `MQTT_BROKER_HOST`
4. **Joystick**: Edit `joysitck/main/config.h` → `MQTT_BROKER_URL`
5. **Restart all components**

### Adjusting Camera Stream FPS

- **WatchTower** (`watchtower_config.py`): Change `STREAM_FPS` (default 10)
- Lower FPS reduces network/CPU load but increases latency
- Higher FPS improves responsiveness but may cause lag on slow networks

### Tuning Air Mouse Sensitivity

- **Qt App**: Settings page → "Air Mouse Sensitivity" slider (saved to `config.json`)
- **Joystick**: Adjust gyro scale factor in `joysitck/main/sensor_task.c`
- **Qt Smoothing**: Modify `airmouse_manager.cpp` → smoothing window size
