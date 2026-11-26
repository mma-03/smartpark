
/*
ENSC 351 - Design Project - SmartPark
Author: Laxmi Unnithan
HAL module: IR Sensor

reads the KY-033 IR sensors using GPIO, filters unstable readings
with debouncing, and counts new vehicle detection events.
*/

#include "hal/ir_sensor.h"

#include <gpiod.h>
#include <stdio.h>
#include <string.h>

// require three matching readings to accept a change
#define DEBOUNCE_SAMPLES 3

void IrSensor_init(IrSensor *sensor, int activeLevel)
{
    if (sensor == NULL) {
        return;
    }

    memset(sensor, 0, sizeof(*sensor));

    sensor->activeLevel = activeLevel;
    sensor->lastReading = -1;
    sensor->pendingReading = -1;
}

int IrSensor_update(IrSensor *sensor, int reading)
{
    if (sensor == NULL || (reading != 0 && reading != 1)) {
        return -1;
    }

    // first reading establishes the initial state without counting
    if (!sensor->started) {
        sensor->lastReading = reading;
        sensor->pendingReading = reading;
        sensor->started = 1;
        return 0;
    }

    // reset debounce count if reading matches stable state
    if (reading == sensor->lastReading) {
        sensor->pendingReading = reading;
        sensor->sameCount = 0;
        return 0;
    }

    // check for three consecutive readings of a new state
    if (reading != sensor->pendingReading) {
        sensor->pendingReading = reading;
        sensor->sameCount = 1;
    } else {
        sensor->sameCount++;
    }

    if (sensor->sameCount < DEBOUNCE_SAMPLES) {
        return 0;
    }

    sensor->lastReading = reading;
    sensor->sameCount = 0;

    // count a detection only when the sensor becomes active
    if (reading == sensor->activeLevel) {
        sensor->eventCount++;
        return 1;
    }

    return 0;
}

int IrSensor_isPresent(const IrSensor *sensor)
{
    if (sensor == NULL || !sensor->started) {
        return 0;
    }

    return sensor->lastReading == sensor->activeLevel;
}

int IrSensor_getCount(const IrSensor *sensor)
{
    if (sensor == NULL) {
        return 0;
    }

    return sensor->eventCount;
}

int IrSensor_gpioInit(IrSensor *sensor,
                      const char *chipPath,
                      unsigned int line)
{
    if (sensor == NULL || chipPath == NULL) {
        return -1;
    }

    sensor->gpioLine = line;

    sensor->chip = gpiod_chip_open(chipPath);

    if (sensor->chip == NULL) {
        perror("failed to open GPIO chip");
        return -1;
    }

    struct gpiod_line_settings *settings =
        gpiod_line_settings_new();

    struct gpiod_line_config *config =
        gpiod_line_config_new();

    if (settings == NULL || config == NULL) {
        fprintf(stderr, "failed to allocate GPIO settings\n");
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(config);
        IrSensor_gpioCleanup(sensor);
        return -1;
    }

    int result = gpiod_line_settings_set_direction(
        settings, GPIOD_LINE_DIRECTION_INPUT
    );

    if (result == 0) {
        result = gpiod_line_config_add_line_settings(
            config, &line, 1, settings
        );
    }

    if (result == 0) {
        sensor->request = gpiod_chip_request_lines(
            sensor->chip, NULL, config
        );
    }

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(config);

    if (result != 0 || sensor->request == NULL) {
        perror("failed to request GPIO input");
        IrSensor_gpioCleanup(sensor);
        return -1;
    }

    return 0;
}

int IrSensor_gpioRead(IrSensor *sensor)
{
    if (sensor == NULL || sensor->request == NULL) {
        return -1;
    }

    enum gpiod_line_value value =
        gpiod_line_request_get_value(
            sensor->request, sensor->gpioLine
        );

    if (value == GPIOD_LINE_VALUE_ACTIVE) {
        return 1;
    }

    if (value == GPIOD_LINE_VALUE_INACTIVE) {
        return 0;
    }

    return -1;
}

void IrSensor_gpioCleanup(IrSensor *sensor)
{
    if (sensor == NULL) {
        return;
    }

    if (sensor->request != NULL) {
        gpiod_line_request_release(sensor->request);
        sensor->request = NULL;
    }

    if (sensor->chip != NULL) {
        gpiod_chip_close(sensor->chip);
        sensor->chip = NULL;
    }
}
