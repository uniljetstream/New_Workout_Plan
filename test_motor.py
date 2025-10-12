#!/usr/bin/env python3
"""
Jetson Nano - STM32 - MG996R Servo Motor Test Script
Tests UART communication and pan-tilt servo motor control
"""

import serial
import time
import sys

# Configuration
UART_PORT = '/dev/ttyUSB0'  # Change to /dev/ttyACM0 if needed
BAUDRATE = 115200
TIMEOUT = 1

def test_uart_connection():
    """Test UART connection to STM32"""
    print("=" * 50)
    print("Testing UART Connection...")
    print("=" * 50)

    try:
        ser = serial.Serial(UART_PORT, BAUDRATE, timeout=TIMEOUT)
        print(f"✓ Successfully opened {UART_PORT}")
        print(f"  Baudrate: {BAUDRATE}")
        print(f"  Timeout: {TIMEOUT}s")
        return ser
    except serial.SerialException as e:
        print(f"✗ Failed to open {UART_PORT}")
        print(f"  Error: {e}")
        print("\nTroubleshooting:")
        print("  1. Check if device is connected: ls /dev/ttyUSB* /dev/ttyACM*")
        print("  2. Check permissions: sudo chmod 666 /dev/ttyUSB0")
        print("  3. Add user to dialout group: sudo usermod -a -G dialout $USER")
        return None

def send_command(ser, command):
    """Send command to STM32 and wait for response"""
    if not ser:
        return False

    try:
        # Send command
        cmd = f"{command}\n"
        ser.write(cmd.encode())
        print(f"  → Sent: {command}")

        # Wait a bit for motor to move
        time.sleep(0.5)

        # Read response (if any)
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"  ← Response: {response.strip()}")

        return True
    except Exception as e:
        print(f"  ✗ Error sending command: {e}")
        return False

def test_center_position(ser):
    """Test center position (0, 0)"""
    print("\n" + "=" * 50)
    print("Test 1: Center Position (0, 0)")
    print("=" * 50)
    send_command(ser, "CENTER")
    time.sleep(2)

def test_pan_movement(ser):
    """Test pan movement (left-right)"""
    print("\n" + "=" * 50)
    print("Test 2: Pan Movement (Left-Right)")
    print("=" * 50)

    angles = [0, 30, 60, 30, 0, -30, -60, -30, 0]

    for angle in angles:
        print(f"\nPan angle: {angle}°")
        send_command(ser, f"PAN:{angle}")
        time.sleep(1.5)

def test_tilt_movement(ser):
    """Test tilt movement (up-down)"""
    print("\n" + "=" * 50)
    print("Test 3: Tilt Movement (Up-Down)")
    print("=" * 50)

    angles = [0, 30, 60, 30, 0, -30, -60, -30, 0]

    for angle in angles:
        print(f"\nTilt angle: {angle}°")
        send_command(ser, f"TILT:{angle}")
        time.sleep(1.5)

def test_combined_movement(ser):
    """Test combined pan-tilt movement"""
    print("\n" + "=" * 50)
    print("Test 4: Combined Pan-Tilt Movement")
    print("=" * 50)

    positions = [
        (0, 0),      # Center
        (30, 30),    # Upper right
        (30, -30),   # Lower right
        (-30, -30),  # Lower left
        (-30, 30),   # Upper left
        (0, 0),      # Back to center
    ]

    for pan, tilt in positions:
        print(f"\nPosition: Pan={pan}°, Tilt={tilt}°")
        send_command(ser, f"PANTILT:{pan},{tilt}")
        time.sleep(2)

def test_full_range(ser):
    """Test full range movement"""
    print("\n" + "=" * 50)
    print("Test 5: Full Range Movement (-60° to +60°)")
    print("=" * 50)
    print("WARNING: Make sure motors have clearance!")

    response = input("Continue with full range test? (y/n): ")
    if response.lower() != 'y':
        print("Skipped full range test")
        return

    # Test pan full range
    print("\nPan full range:")
    for angle in [-60, -30, 0, 30, 60, 0]:
        print(f"  Pan: {angle}°")
        send_command(ser, f"PAN:{angle}")
        time.sleep(1.5)

    # Test tilt full range
    print("\nTilt full range:")
    for angle in [-60, -30, 0, 30, 60, 0]:
        print(f"  Tilt: {angle}°")
        send_command(ser, f"TILT:{angle}")
        time.sleep(1.5)

def test_stop_command(ser):
    """Test stop command"""
    print("\n" + "=" * 50)
    print("Test 6: Stop Command")
    print("=" * 50)
    send_command(ser, "STOP")
    time.sleep(1)

def interactive_mode(ser):
    """Interactive mode for manual testing"""
    print("\n" + "=" * 50)
    print("Interactive Mode")
    print("=" * 50)
    print("Commands:")
    print("  PAN:<angle>        - Set pan angle (-60 to 60)")
    print("  TILT:<angle>       - Set tilt angle (-60 to 60)")
    print("  PANTILT:<pan>,<tilt> - Set both angles")
    print("  CENTER             - Return to center (0, 0)")
    print("  STOP               - Stop motors")
    print("  quit               - Exit interactive mode")
    print()

    while True:
        try:
            cmd = input("Enter command: ").strip()
            if cmd.lower() == 'quit':
                break
            if cmd:
                send_command(ser, cmd)
        except KeyboardInterrupt:
            print("\nExiting interactive mode...")
            break

def main():
    """Main test function"""
    print("\n" + "=" * 50)
    print("Jetson Nano - MG996R Servo Motor Test")
    print("=" * 50)
    print()

    # Test UART connection
    ser = test_uart_connection()
    if not ser:
        sys.exit(1)

    try:
        # Run tests
        test_center_position(ser)
        time.sleep(1)

        test_pan_movement(ser)
        time.sleep(1)

        test_tilt_movement(ser)
        time.sleep(1)

        test_combined_movement(ser)
        time.sleep(1)

        test_full_range(ser)
        time.sleep(1)

        test_stop_command(ser)

        # Interactive mode
        print("\n" + "=" * 50)
        response = input("Enter interactive mode? (y/n): ")
        if response.lower() == 'y':
            interactive_mode(ser)

        # Return to center before exit
        print("\nReturning to center position...")
        send_command(ser, "CENTER")

    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
        print("Returning to center position...")
        send_command(ser, "CENTER")

    finally:
        if ser:
            ser.close()
            print("\n✓ Serial port closed")

    print("\n" + "=" * 50)
    print("Test completed!")
    print("=" * 50)

if __name__ == "__main__":
    main()
