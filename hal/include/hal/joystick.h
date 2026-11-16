/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
HAL module: Joystick

provides functions to initialize the joystick, read the MCP3208 ADC,
and determine joystick direction.
*/

#ifndef HAL_JOYSTICK_H
#define HAL_JOYSTICK_H

// directions detected from the joystick axes
typedef enum{
    JOYSTICK_NONE,
    JOYSTICK_UP,
    JOYSTICK_DOWN,
    JOYSTICK_LEFT,
    JOYSTICK_RIGHT
} JoystickDirection;

void Joystick_init(void);
int Joystick_read(int channel);
JoystickDirection Joystick_getDirection(void);
void Joystick_cleanup(void);

#endif
