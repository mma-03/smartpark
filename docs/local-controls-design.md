# Local Controls Design — Joystick and Rotary Encoder

**Author:** Anushka Kulasekara  

## 1. Overview

The SmartPark system will use a joystick and rotary encoder to provide local administrative controls for the parking garage. These components will allow an administrator to manually operate the entrance gate, select parking spaces, change parking availability, and request camera calibration without using the web interface.

The joystick will interface with the BeagleY-AI through the MCP3208 ADC, while the rotary encoder will use GPIO pins.

## 2. Joystick Controls

The joystick will control the entrance gate and operating modes.

| Joystick Input | Function |
|---|---|
| UP | Manually open the entrance gate |
| DOWN | Manually close the entrance gate |
| LEFT | Switch to automatic gate mode |
| RIGHT | Switch to manual gate mode |
| CENTER | No action |

### Hardware Interface

- X-axis connected to MCP3208 CH0.
- Y-axis connected to MCP3208 CH1.
- MCP3208 communicates through SPI (`/dev/spidev0.0`).
- Joystick powered using 3.3 V and GND.

ADC readings will determine the joystick direction using predefined thresholds. Direction changes will be detected to prevent repeated commands when the joystick is held.

Gate opening and closing commands will operate in manual mode and follow the system's safety checks.

## 3. Rotary Encoder Controls

The rotary encoder will allow an administrator to select and manage individual parking spaces.

| Rotary Input | Function |
|---|---|
| Clockwise rotation | Select next parking space |
| Counterclockwise rotation | Select previous parking space |
| Short button press | Block/unblock selected parking space |
| Long button press | Request camera calibration (optional) |

### Hardware Interface

- Channel A connected to GPIO16.
- Channel B connected to GPIO17.
- Push button (SW) connected to an available GPIO pin (TBD).
- GPIO access through `libgpiod`.
- Hardware/software debounce to prevent false readings.

These connections are based on Assignment 2 and will be verified against other SmartPark hardware.

### Parking Space Selection

The program will maintain the currently selected parking space. Clockwise rotation increments the selection, while counterclockwise rotation decrements it. Selection will wrap around between the first and last spaces.

Pressing the encoder button will toggle the selected space's administrative blocked status.

A blocked space will remain unavailable even if the camera detects that it is empty.

## 4. Software Design

The local controls will follow the project's HAL and application structure.

### Hardware Abstraction Layer (HAL)

**joystick.h / joystick.c**
- Initialize SPI communication with the MCP3208.
- Read joystick X and Y ADC values.
- Determine and return joystick direction.

**rotary.h / rotary.c**
- Initialize rotary encoder GPIO inputs.
- Detect clockwise/counterclockwise rotation.
- Detect push-button presses.
- Handle input debounce.

### Application Layer

**local_controls.h / local_controls.c**
- Process joystick and rotary encoder inputs.
- Maintain the selected parking space.
- Request gate opening/closing and operating mode changes.
- Update administrative parking-space availability.
- Request camera calibration.

The HAL modules will only handle hardware inputs. The application layer will communicate with the appropriate SmartPark modules.

## 5. Integration With SmartPark

### Gate Control

Joystick commands will communicate with the servo gate control module. Automatic mode will follow normal parking availability logic, while manual mode will allow administrator commands subject to safety checks.

### Parking Availability

The rotary encoder will allow administrators to select and block/unblock parking spaces.

Parking availability will depend on both camera-detected occupancy and administrative blocking. Changes should be reflected on the webpage and status LEDs.

### Camera Calibration

A long encoder button press may request camera calibration through the vision module. Calibration will require confirmation that the garage is empty before updating the reference image.

## 6. Testing Plan

1. Verify joystick SPI communication and ADC readings.
2. Test all joystick directions and gate mode commands.
3. Verify rotary encoder GPIO connections.
4. Test clockwise/counterclockwise rotation and debounce.
5. Test short and long button presses.
6. Verify parking-space selection and wraparound.
7. Test blocking/unblocking parking spaces.
8. Verify manual gate operation and automatic mode.
9. Test camera calibration requests.
10. Integrate local controls with the remaining SmartPark modules.


**Note:** The Initial development will focus on adapting the joystick code from Assignment 1 and rotary encoder code from Assignment 2.

