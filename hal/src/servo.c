/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
HAL module: Servo

based on the pwm implementation from Assignment 2.
controls the MG90S servo motor using hardware PWM and moves
the entrance gate between the open and closed positions.
*/

#include "hal/servo.h"
#include <stdio.h>
#include <stdbool.h>

// pwm interface used to control the servo motor
#define PWM_PATH "/dev/hat/pwm/GPIO12/"
#define PWM_PERIOD PWM_PATH "period"
#define PWM_DUTY_CYCLE PWM_PATH "duty_cycle"
#define PWM_ENABLE PWM_PATH "enable"

// servo period and initial pulse widths in nanoseconds
#define SERVO_PERIOD_NS 20000000
#define SERVO_CLOSED_NS 1000000
#define SERVO_OPEN_NS 1500000

// pwm enable and disable values
#define PWM_DISABLED 0
#define PWM_ENABLED 1

// keep track of the servo initialization and pwm status
static bool servoInitialized = false;
static bool pwmEnabled = false;

// write a value to the specified pwm file
static int writePWM(const char *path, int value){

    // open the pwm file for writing
    FILE *file = fopen(path, "w");
    if (file == NULL){
        perror(path);
        return -1;
    }

    // write the requested value to the pwm file
    if (fprintf(file, "%d\n", value) < 0){
        perror("Servo PWM write");
        fclose(file);
        return -1;
    }

    // close the file and check that the value was written
    if (fclose(file) != 0){
        perror("Servo PWM close");
        return -1;
    }

    return 0;
}

int Servo_init(void){

    // prevent the servo from being initialized more than once
    if (servoInitialized){
        return 0;
    }

    // disable pwm before changing its configuration
    if (writePWM(PWM_ENABLE, PWM_DISABLED) < 0){
        return -1;
    }

    // reset the duty cycle before changing the period
    if (writePWM(PWM_DUTY_CYCLE, 0) < 0){
        return -1;
    }

    // set the pwm period to 20 ms for a frequency of 50 Hz
    if (writePWM(PWM_PERIOD, SERVO_PERIOD_NS) < 0){
        return -1;
    }

    // leave pwm disabled until a gate movement is requested
    servoInitialized = true;
    pwmEnabled = false;

    return 0;
}

// set the pulse width used to determine the servo position
static int setServoPosition(int pulseWidth){

    // reject movement before the servo has been initialized
    if (!servoInitialized){
        return -1;
    }

    // update the pulse width to move the servo
    if (writePWM(PWM_DUTY_CYCLE, pulseWidth) < 0){
        return -1;
    }

    // enable pwm if this is the first movement request
    if (!pwmEnabled){
        if (writePWM(PWM_ENABLE, PWM_ENABLED) < 0){
            return -1;
        }

        pwmEnabled = true;
    }

    return 0;
}

int Servo_open(void){

    // move the servo to the open gate position
    return setServoPosition(SERVO_OPEN_NS);
}

int Servo_close(void){

    // move the servo to the closed gate position
    return setServoPosition(SERVO_CLOSED_NS);
}

void Servo_cleanup(void){

    // disable pwm if the servo was initialized
    if (servoInitialized){
        if (writePWM(PWM_ENABLE, PWM_DISABLED) < 0){
            return;
        }

        servoInitialized = false;
        pwmEnabled = false;
    }
}
