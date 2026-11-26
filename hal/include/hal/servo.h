/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
HAL module: Servo

controls the MG90S servo motor using hardware PWM.
moves the entrance gate between the open and closed positions.
*/

#ifndef SERVO_H
#define SERVO_H

// initialize the pwm interface used by the servo
int Servo_init(void);

// move the servo to the open position
int Servo_open(void);

// move the servo to the closed position
int Servo_close(void);

// disable the pwm output
void Servo_cleanup(void);

#endif
