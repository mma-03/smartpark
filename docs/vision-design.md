# SmartPark Vision Design Notes
**Author:** Muhammad Maaz Anwar

## Purpose
- Uses a USB Webcam to continuously observe the parking spaces and decide which spots are occupied.
- Reports to callers which parking spots are occupied, and when each one last changed.
- It does NOT drive the LEDs or the servo gate, or read the entry/exit sensors.

## System Context

### Callers
- **LEDs:** need which spots are occupied, every ~1s
- **Gate logic:** needs which spots are occupied, every ~1s
- **Webpage:** needs which spots are occupied, every ~1s
- **Parking duration:** needs which spots are occupied and the time of each spot's last change, every ~1s.
- **Event log:** needs each spot's state plus the time of it's last change, every ~1s

### Inputs to vision
**Calibration:** Detection compares each parking spot to when it was empty. In order to do that, the program needs a saved picture of the empty lot as a reference frame.
- Needs to happen at setup when the lot is empty.
- Needs to happen whenever the scene changes for reasons other than cars: room lighting, sunlight, bumping the camera.
- Local admin controls needs to be able to say that the lot is empty, calibrate now.

**Spot Layout:** The camera only produces pixels. The program needs each spot as a rectangle in pixel coordinates (where it's top-left corner is, plus its width and height).
- Save one frame as a PGM.
- Open it in GIMP to see the pixel coordinates.
- Save the coordinates in a small text file that the program reads at startup.
- If the camera moves, re-measure and edit the text file, no code changes.

## Decision Log
- **Decision:** This module only reports per-spot occupancy, it does not report whether the lot is full. **Why:** "Full" also depends on admin-marked unavailable spots, which this module doesn't know about. The part that tracks unavailable spots combined both to decide "full".
- **Decision:** Admin-marked unavailable spots are handled outside of  the vision module. **Why:** They are set by an admin through the webpage, not detected from the image. The webpage, LEDs, and gate logic use them.
- **Decision:** Vision provides no audio output. **Why:** The "garage full" warning uses the same "full" decision as the gate.
- **Decision:** Calibration is triggered manually from the local admin controls. **Why:** Only a person can confirm the lot is actually empty, and lighting changes or camera movement require a new reference frame.
- **Decision:** Parking spot rectangles are loaded from a textfile at startup. **Why:** If the camera moves, the coordinates can be remeasured and edited without changing code.
- **Decision:** Take a reading every T = 150 ms, a spot changes state only after R = 5 readings in a row agree, so it updates in R x T = 750 ms. **Why:** The update must be under ~1 s so the gate doesn't let a car into a full lot, and longer than a short distrubance such as a hand or another object passing over a spot. N >= 2 so one bad frame can't incorrectly mark a spot. T >= 33 ms because the camera only produces a new frame every 33 ms.
- **Decision:** Vision should record, for each spot, the time of its last state change and report it to callers. **Why:** Only vision knows when the car actually arrived: the time of the first reading in the R = 5 readings. A caller checking every ~1 s would see a chane up to ~1.75 s late (750 ms debounce + ~ 1s polling).
- **Decision:** Keep `camera` separate from `occupancy`. **Why:** Detection can then be tested on a PC with PGM images instead of a camera.

## Modules
| Lives in | Module | Job |
|---|---|---|
| `hal/` | `camera` | Opens the webcam and hands back grayscale frames. Only place where code touches the camera. |
| `app/` | `occupancy` | Compares each spot to the reference frame, debounces, and records change times. Pure logic without touching hardware. |
| `app/` | `pgm` | Reads + writes grayscale images as PGM files, for testing snapshots. |
| `app/` | `vision` | Loads the spot file, runs the 150ms loop in a thread, does calibration. Only module that callers use. |

## Functions
**Convention:** functions return `0` on success and `-1` on failure. Caller always provides the memory, no modules uses `malloc`.

### hal/camera

### app/pgm

### app/occupancy

### app/vision

