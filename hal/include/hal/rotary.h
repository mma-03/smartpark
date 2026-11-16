/*
ENSC 351 - Design Project -SmartPark
Author: Anushka Kulasekara
HAL module: Rotary encoder

Reads rotary encoder rotation and its push button.
*/

#ifndef ROTARY_H
#define ROTARY_H

// Button events returned after the button is released.
// referenced code from Assignment 2
#define ROTARY_BUTTON_NONE  0
#define ROTARY_BUTTON_SHORT 1
#define ROTARY_BUTTON_LONG  2

// buttonLine is the gpiochip2 line offset for SW.
// pass -1 to use rotation without the push button for now.
void Rotary_init(int buttonLine);

// returns 1 for clockwise, -1 for counterclockwise, or 0.
int Rotary_getDirection(void);

// returns ROTARY_BUTTON_NONE, SHORT, or LONG.
int Rotary_getButtonEvent(void);

void Rotary_cleanup(void);

#endif
