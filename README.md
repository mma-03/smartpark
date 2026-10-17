# SmartPark: Mini Smart Parking Garage

ENSC 351 (Fall 2026) term project on the BeagleY-AI. A tabletop parking garage
that detects cars at the entrance/exit, uses a USB webcam with object detection
to find free spaces, controls a servo gate, and offers a web interface for
monitoring and admin control.

## Team
- Laxmi Unnithan
- Muhammad Maaz Anwar
- Anushka Kulasekara

## Hardware
- BeagleY-AI
- USB webcam
- entry/exit proximity sensors
- red/green LEDs,
- servo motor
- joystick
- rotary encoder
- USB audio

## Repository layout
- `app/`: program logic (garage state, gate decisions, event log)
- `hal/`: one module per piece of hardware
- `vision/`: webcam and object detection
- `web/`: webpage and its server
- `deploy/`: board setup (auto-start, watchdog)
- `docs/`: proposal, ms reports, diagrams

## Building
    cmake -S . -B build -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc
    cmake --build build
