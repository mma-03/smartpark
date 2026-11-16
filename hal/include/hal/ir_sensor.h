#ifndef IR_SENSOR_H
#define IR_SENSOR_H

// Stores the state of one IR sensor (entrance or exit).
typedef struct {
    int active_level;      // Raw GPIO value when an object is detected
    int initialized;       // Have we read this sensor before?
    int stable_present;    // Last confirmed state (0 or 1)
    int pending_present;   // Possible next state
    int pending_samples;   // Consecutive readings of that state
    int events;            // Number of new detections
} IrSensor;

void IrSensor_init(IrSensor *sensor, int active_level);
// Supply a raw 0 or 1 from GPIO (or a test).
// Returns 1 for a new detection, 0 otherwise, -1 for invalid input.
int IrSensor_update(IrSensor *sensor, int raw_level);
int IrSensor_isPresent(const IrSensor *sensor);
int IrSensor_getCount(const IrSensor *sensor);

#endif
