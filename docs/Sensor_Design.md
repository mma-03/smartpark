# Entrance and Exit IR Sensors

## Overview

I'm using two KY-033 IR sensors for the garage entrance and exit. Each sensor gives a digital reading (0 or 1). The goal is to detect when a car passes through either side of the garage and send that information to the rest of the system. The camera module will handle checking which individual parking spots are occupied.

## Progress so far

I've started writing the IR sensor module in C. The current code processes readings provided to it and keeps track of detections. It does not read the physical sensors yet.

So far, the code:

- Stores the sensor state and number of detections in an `IrSensor` struct.
- Uses the first reading to set the initial state without counting an event.
- Waits for three consecutive readings showing a change before updating the confirmed state.
- Counts a new event when the state changes from clear to detected.
- Avoids counting the same detection repeatedly while the sensor stays active.
- Lets another event be counted once the sensor returns to the clear state.

The three-reading check is just an initial approach to filtering inconsistent readings. I'll need to test whether it works well when a toy car moves past the sensor.

## Files and functions

| File | Purpose |
| --- | --- |
| `hal/ir_sensor.h` | Defines the `IrSensor` structure and function declarations. |
| `hal/ir_sensor.c` | Handles sensor initialization, reading changes, and detection counts. |
| `tests/test_ir_sensor.c` | Tests the detection logic with fixed input values. |

The functions written so far are:

- `IrSensor_init()` — initializes the sensor state.
- `IrSensor_update()` — processes one reading and reports a new detection when confirmed.
- `IrSensor_isPresent()` — returns the last confirmed detection state.
- `IrSensor_getCount()` — returns the number of confirmed detection events.

`IrSensor_update()` returns `1` for a new detection, `0` for no new event, and `-1` if the input is invalid.

## Testing

I've tested the basic logic using fixed 0/1 inputs. The tests check that a detection is counted once, that the sensor can clear, and that another detection can be counted afterward. There is also a check for invalid input values. 

## Next steps

- Test the module with the actual sensors.
- Check whether the detection level and three-reading filter work reliably with toy cars.
- Pass entrance and exit events to the main garage application.
- Coordinate with the event/timing and gate modules when integrating the sensors.


