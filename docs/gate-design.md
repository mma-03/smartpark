# Gate Control Design — Servo Motor

**Author:** Anushka Kulasekara

## 1. Overview

The SmartPark system will use a Tower Pro MG90S servo motor to control the entrance gate of the parking garage. The gate will support automatic and manual operation based on parking availability, vehicle detection, and administrator input.

The servo motor will interface with the BeagleY-AI through hardware PWM.

## 2. Gate Control

### Automatic Mode

- Open the gate when a vehicle is detected and parking space is available.
- Prevent vehicle entry when the parking garage is full.
- Allow vehicles to exit regardless of parking availability.
- Close the gate once the vehicle has passed and the gate area is clear.
- Prevent the gate from closing when an obstruction is detected.

### Manual Mode

The joystick will allow an administrator to manually control the entrance gate and switch between operating modes.

| **Joystick Input** | **Function** |
| --- | --- |
| UP | Manually open the entrance gate |
| DOWN | Manually close the entrance gate |
| LEFT | Switch to automatic gate mode |
| RIGHT | Switch to manual gate mode |
| CENTER | No action |

Manual gate operation will follow the system's safety checks.

## 3. Hardware Design

The selected servo motor is the Tower Pro MG90S 180-degree metal gear micro servo from Lee's Electronics.

### Hardware Interface

- Signal wire (orange) connected to GPIO12 (Physical Pin 32).
- Power wire (red) connected to a regulated 5 V supply.
- Ground wire (brown) connected to common GND.
- Servo controlled using a 3.3 V PWM signal, subject to compatibility testing.

These connections are based on the PWM implementation from Assignment 2 and will be verified against other SmartPark hardware.

The servo will use a 5 V power supply while sharing a common ground with the BeagleY-AI.

### PWM Configuration

| **Parameter** | **Initial Value** |
| --- | --- |
| PWM frequency | 50 Hz |
| PWM period | 20000000 ns |
| Closed position (0 degrees) | 1000000 ns |
| Open position (90 degrees) | 1500000 ns |

Hardware PWM will be accessed through `/dev/hat/pwm/GPIO12/`.

The gate arm will be attached to the servo horn. The pulse widths will be calibrated to achieve the required open and closed positions.

## 4. Software Design

The gate control module will follow the project's HAL and application structure.

### Hardware Abstraction Layer (HAL)

**servo.h / servo.c**

- Initialize and configure hardware PWM.
- Adjust the PWM pulse width.
- Move the servo between open and closed positions.
- Release PWM resources during shutdown.

### Application Layer

**gate.h / gate.c**

- Manage automatic and manual operating modes.
- Process gate opening and closing requests.
- Check parking availability and vehicle sensor readings.
- Maintain the estimated gate state.
- Communicate with the servo HAL.

The HAL module will only handle physical servo control. The application layer will determine when the gate should open or close.

Gate operations will be non-blocking to prevent interruptions to other SmartPark modules.

## 5. Integration With SmartPark

### Local Controls

The joystick will communicate with the gate control module to request gate movement and operating mode changes.

The rotary encoder will indirectly affect automatic gate operation by changing the administrative availability of parking spaces.

### Parking Availability

The gate controller will determine parking availability using both camera-detected occupancy and administrative blocking.

A blocked parking space will remain unavailable even if the camera detects that it is empty.

### Camera Calibration

Automatic vehicle entry will be temporarily suspended during camera calibration or when valid occupancy information is unavailable.

### Vehicle Detection

Vehicle sensors will communicate with the gate controller to determine when a vehicle is approaching or passing through the gate.

Automatic closing will require reliable detection that the gate area is clear.

### Web Interface and Status LEDs

The gate state, operating mode, and parking availability should be reflected on the webpage.

Web-based gate commands, if implemented, will communicate with the same gate controller used by the local controls.

The gate controller and status LEDs will use the same parking availability information.

## 6. Testing Plan

1. Verify servo wiring and PWM configuration.
2. Test servo movement independently.
3. Calibrate the open and closed gate positions.
4. Test manual gate operation using the joystick.
5. Verify switching between automatic and manual modes.
6. Test automatic gate operation based on parking availability.
7. Verify gate operation when parking spaces are blocked/unblocked.
8. Test vehicle detection and gate safety checks.
9. Verify gate status updates on the webpage.
10. Integrate gate control with the remaining SmartPark modules.

**Note:** The initial development will focus on adapting the PWM code from Assignment 2 and testing the servo motor independently before integrating it with the remaining SmartPark modules.
