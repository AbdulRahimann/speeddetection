# Speed Detection System

This project implements a compact speed measurement system using two reflective optical sensors and an ESP32 microcontroller. It detects the time between two sensor triggers, calculates the speed of an object crossing the sensor pair, and reports the result over the serial monitor.

## Features

- Dual-sensor timing setup for speed measurement
- ESP32-based interrupt-driven detection
- Debounce logic to avoid false triggers
- Timeout handling for incomplete measurements
- Output filtering for out-of-range readings
- Serial reporting of speed, direction, and timing data

## Hardware

- ESP32 development board
- 2x LM393 reflective optical sensors
- Wiring as follows:

| Component | ESP32 Pin |
|-----------|-----------|
| Sensor 1 | GPIO 18 |
| Sensor 2 | GPIO 19 |

The sensors are configured with internal pull-ups and are expected to produce a falling edge when an object is detected.

## Working Principle

The two sensors are positioned a known distance apart. When an object passes the first sensor, the system records the timestamp. When it passes the second sensor, the second timestamp is recorded. The elapsed time is used to compute the speed:

- Distance between sensors: 20.0 m
- Speed formula: speed = distance / time
- Conversion to km/h: speed_m_s * 3.6

The firmware treats the passage direction as either:

- `1_TO_2` if sensor 1 is triggered first
- `2_TO_1` if sensor 2 is triggered first

## Configuration

The main timing constants are defined near the top of the sketch:

- `SENSOR1_PIN = 18`
- `SENSOR2_PIN = 19`
- `SENSOR_DISTANCE_M = 20.0f`
- `TIMEOUT_US = 2000000UL`
- `DEBOUNCE_US = 3000UL`
- `CLEAR_STABLE_MS = 100`
- `MAX_EXPECTED_SPEED_KMH = 5.0f`

These values can be adjusted depending on the exact sensor placement, object size, and expected speed range.

## Serial Output

When a valid reading is captured, the firmware prints data in this format:

```text
SPEED,1_TO_2,12.345,0.50,1.80
```

Fields are:

- `SPEED` or `OUT_OF_RANGE`
- direction (`1_TO_2` or `2_TO_1`)
- elapsed time in milliseconds
- speed in meters/second
- speed in km/h

Example for an out-of-range reading:

```text
OUT_OF_RANGE,2_TO_1,7.42
```

Additional status messages include:

- `SYSTEM_READY`
- `ESP32 speed detector started`
- `TIMEOUT`
- `INVALID_MEASUREMENT`
- `READY`

## Setup and Upload

1. Open the project in the Arduino IDE or VS Code with Arduino support.
2. Select the correct ESP32 board and COM port.
3. Compile and upload `speeddetection.ino`.
4. Open the serial monitor at 115200 baud.
5. Trigger the sensor pair with an object crossing between them.

## Notes

- The system waits until both sensors are clearly in the HIGH state before resetting and preparing for the next measurement.
- The code uses interrupts for fast timing and uses a critical section to protect shared variables from race conditions.
- The `MAX_EXPECTED_SPEED_KMH` limit prevents reporting unrealistic readings caused by noise or invalid triggering.

## Project Files

- `speeddetection.ino` — main firmware implementation
- `README.md` — project documentation

## License

This project is provided as-is for educational and experimental use.
