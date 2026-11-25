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
Give `vision` grayscale frames from the webcam. Only code that touches the camera. `vision` needs `camera`:
1. Once at startup: initialize the camera
2. Every 150 ms: give the latest frame
3. Once at shutdown: release the camera

```c
// returns 0/-1, opens the camera, set YUYV at requested size, start streaming, discard warmup frames.
int camera_init(const char *device_path, int width, int height);
// returns 0/-1, wait for the next frame, and copy its brightness bytes into 'out'.
int camera_get_frame(Frame *out);
// stop streaming, release buffers, and close the device.
void camera_cleanup(void);
```

**Notes:**
- `camera_init` fails if the driver returns a different size than requested since spot coordinates are measured at one size. A different size would give wrong results.
- `camera_init` discards the first frames, because the first ~2s of frames are black while the camera adjusts.

### app/pgm
Saves a grayscale frame to a PGM file, and loads one back. Holds nothing between calls.

```c
// returns 0/-1, writes the PGM header then the pixels to the file at 'path'
int pgm_write(const char *path, const Frame *frame);
// returns 0/-1, reads the file at 'path' into 'out'
int pgm_read(const char *path, Frame *out);
```

**Notes:**
- `pgm_read` checks the file starts with `P5` (grayscale PGM) and the max value is 255, otherwise it fails.
- `pgm_read` fails if width x height is bigger than the maximum frame size, so it never overflows the caller's buffer.


### app/occupancy
Decides whether each spot is occupied, debounce the result, and record when each spot changes.

**When does `vision` use it?**
1. At startup: gives parking spots
2. On calibration: gives the empty lot
3. Every 150 ms: new frame and the time
4. When a caller asks: each spot's status

**What it needs:**
- spot rectangles
- reference frame (empty lot)
- each new frame
- the current time, passed in by the caller

```c
// returns 0/-1, stores and resets all to empty. Fails if a spot is outside the frame, or there are too many.
int occupancy_init(const SpotRect spots[], int count, int frame_width, int frame_height);
// returns 0/-1, copies 'frame' as the empty-lot reference and resets debounce.
int occupancy_set_reference(const Frame *frame);
// returns 0/-1. compares every spot to the reference and debounces. Fails if no reference set.
int occupancy_update(const Frame *frame, int64_t now_ms);
// copies each spot's status into 'out', returns the number of spots or -1.
int occupancy_get(SpotStatus out[], int max);
```

**Notes:**
- `occupancy_get` returns a count instead of 0.
- Threshold, debounce count (5), and max spots are named constants.

### app/vision
Loads the spot file, runs the 150 ms loop in a thread, handles calibration, and gives callers copies of the results. When does SmartPark use it?
1. At startup: start watching the lot
2. Any time: each spot's status (LED, gate, webpage, duration, event log)
3. When the admin presses calibrate: empty lot
4. For debugging and measuring spots: save a picture
5. at shutdown: stop

**Public:**
```c
// returns 0/-1, loads the spot file, starts the camera and occupancy, starts the thread, requests first calibration
int vision_init(const char *device_path, const char *spot_file);
// thread-safe copy of every spot's status into 'out', returns number of spots or -1
int vision_get_spots(SpotStatus out[], int max);
// requests calibration, the thread uses its next frame as the new reference
void vision_calibrate(void);
// returns 0/-1, saves the latest frame as a PGM  at 'path'
int vision_save_snapshot(const char *path);
// stops the thread, clean up the camera and mutex
void vision_cleanup(void);
```

**Thread loop (every 150 ms):**
1. Get a frame from the camera (no lock needed).
2. Lock, save a copy of the frame, calibrate if needed, or `occupancy_update(frame, now)`.
3. If getting the frame fails, print an error, skip this loop.

**Private:** thread function, `load_spots`, `get_time_ms()`

**Notes:**
- The mutex protects all `occupancy` calls, the latest frame, and the calibration flag.
- Shutdown order: set stop flag, `pthread_join()`, `camera_cleanup()`

## Decision Log
- **Decision:** This module only reports per-spot occupancy, it does not report whether the lot is full. **Why:** "Full" also depends on admin-marked unavailable spots, which this module doesn't know about. The part that tracks unavailable spots combined both to decide "full".
- **Decision:** Admin-marked unavailable spots are handled outside of  the vision module. **Why:** They are set by an admin through the webpage, not detected from the image. The webpage, LEDs, and gate logic use them.
- **Decision:** Vision provides no audio output. **Why:** The "garage full" warning uses the same "full" decision as the gate.
- **Decision:** Calibration is triggered manually from the local admin controls. **Why:** Only a person can confirm the lot is actually empty, and lighting changes or camera movement require a new reference frame.
- **Decision:** Parking spot rectangles are loaded from a text file at startup. **Why:** If the camera moves, the coordinates can be remeasured and edited without changing code.
- **Decision:** Take a reading every T = 150 ms, a spot changes state only after R = 5 readings in a row agree, so it updates in R x T = 750 ms. **Why:** The update must be under ~1 s so the gate doesn't let a car into a full lot, and longer than a short disturbance such as a hand or another object passing over a spot. R >= 2 so one bad frame can't incorrectly mark a spot. T >= 67 ms because the camera only produces a new frame every 67 ms at 176x144.
- **Decision:** Vision should record, for each spot, the time of its last state change and report it to callers. **Why:** Only vision knows when the car actually arrived: the time of the first reading in the R = 5 readings. A caller checking every ~1 s would see a change up to ~1.75 s late (750 ms debounce + ~ 1s polling).
- **Decision:** Keep `camera` separate from `occupancy`. **Why:** Detection can then be tested on a PC with PGM images instead of a camera.

