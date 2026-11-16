#include "ir_sensor.h"
#include <stddef.h>

#define REQUIRED_SAMPLES 3

void IrSensor_init(IrSensor *sensor, int active_level)
{
    if (sensor == NULL) return;
    sensor->active_level = active_level;
    sensor->initialized = 0;
    sensor->stable_present = 0;
    sensor->pending_present = 0;
    sensor->pending_samples = 0;
    sensor->events = 0;
}

int IrSensor_update(IrSensor *sensor, int raw_level)
{
    if (sensor == NULL || (raw_level != 0 && raw_level != 1)) return -1;
    if (sensor->active_level != 0 && sensor->active_level != 1) return -1;

    int present = (raw_level == sensor->active_level);

    // First reading gives the initial state, not a new car event.
    if (sensor->initialized == 0) {
        sensor->initialized = 1;
        sensor->stable_present = present;
        sensor->pending_present = present;
        sensor->pending_samples = 0;
        return 0;
    }

    if (present == sensor->stable_present) {
        sensor->pending_samples = 0;
        return 0;
    }

    // Wait for three matching readings before accepting a change.
    if (present != sensor->pending_present) {
        sensor->pending_present = present;
        sensor->pending_samples = 1;
    } else {
        sensor->pending_samples++;
    }

    if (sensor->pending_samples < REQUIRED_SAMPLES) return 0;

    sensor->stable_present = present;
    sensor->pending_samples = 0;

    if (present == 1) {
        sensor->events++;
        return 1;
    }
    return 0;
}

int IrSensor_isPresent(const IrSensor *sensor)
{
    if (sensor == NULL || sensor->initialized == 0) return 0;
    return sensor->stable_present;
}

int IrSensor_getCount(const IrSensor *sensor)
{
    if (sensor == NULL) return -1;
    return sensor->events;
}
