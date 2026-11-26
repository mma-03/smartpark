
/*
ENSC 351 - Design Project - SmartPark
Author: Laxmi Unnithan
HAL module: IR Sensor

declares the functions used to read the entrance and exit IR sensors,
debounce detection signals, and count vehicle detection events.
*/

#ifndef IR_SENSOR_H
#define IR_SENSOR_H

struct gpiod_chip;
struct gpiod_line_request;

typedef struct {
    int activeLevel;
    int lastReading;
    int pendingReading;
    int sameCount;
    int started;
    int eventCount;

    struct gpiod_chip *chip;
    struct gpiod_line_request *request;
    unsigned int gpioLine;
} IrSensor;

// initialize the sensor's detection logic
void IrSensor_init(IrSensor *sensor, int activeLevel);

// process a sensor reading; returns 1 for a new detection
int IrSensor_update(IrSensor *sensor, int reading);

// return 1 when an object is currently detected
int IrSensor_isPresent(const IrSensor *sensor);

// return the number of detection events
int IrSensor_getCount(const IrSensor *sensor);

// initialize the sensor's GPIO input
int IrSensor_gpioInit(IrSensor *sensor,
                      const char *chipPath,
                      unsigned int line);

// read the GPIO value (0 = LOW, 1 = HIGH)
int IrSensor_gpioRead(IrSensor *sensor);

// close the GPIO connection
void IrSensor_gpioCleanup(IrSensor *sensor);

#endif
