/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
HAL module: Rotary encoder

reads the rotary encoder connected to the BeagleY-AI using libgpiod.
detects clockwise and counter-clockwise rotation and button presses.

*reused code from Assignment 2

*/

#define _POSIX_C_SOURCE 200809L

#include "hal/rotary.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// gpio chip and line numbers used for encoder channels A and B
#define GPIO_CHIP "gpiochip2"
#define GPIO_A 7
#define GPIO_B 8

// bit shifts used to combine the encoder channel readings
#define CHANNEL_A_SHIFT 1
#define PREVIOUS_STATE_SHIFT 2

// encoder returns to state 11 after one physical step
#define ROTARY_REST_STATE 3
#define STEP_THRESHOLD 3

// valid clockwise quadrature transitions
#define CW_11_TO_10 14
#define CW_10_TO_00 8
#define CW_00_TO_01 1
#define CW_01_TO_11 7

// valid counter-clockwise quadrature transitions
#define CCW_11_TO_01 13
#define CCW_01_TO_00 4
#define CCW_00_TO_10 2
#define CCW_10_TO_11 11

// direction values returned to the application
#define DIRECTION_NONE 0
#define DIRECTION_CW 1
#define DIRECTION_CCW -1

// switch logic levels for the active-low push button
#define BUTTON_PRESSED 0
#define BUTTON_RELEASED 1

// button must stay stable for 40 ms to prevent false presses
#define BUTTON_DEBOUNCE_MS 40

// a press lasting 1500 ms or longer is considered a long press
#define LONG_PRESS_MS 1500

// values used to convert monotonic time to milliseconds
#define MS_PER_SECOND 1000LL
#define NS_PER_MS 1000000LL

// store the gpio connections for the encoder and optional push button
static struct gpiod_chip *chip = NULL;
static struct gpiod_line *lineA = NULL;
static struct gpiod_line *lineB = NULL;
static struct gpiod_line *lineSW = NULL;

// keep track of the previous encoder state and valid transitions
static int previousState = 0;
static int steps = 0;

// the push button is active-low: 0 = pressed, 1 = released
static int lastRawButton = BUTTON_RELEASED;
static int stableButton = BUTTON_RELEASED;
static long long lastButtonChangeMs = 0;
static long long pressStartMs = 0;

// return the current monotonic time in milliseconds
// monotonic time is used so clock changes do not affect button timing
static long long getTimeMs(void){
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0){
        perror("Rotary clock");
        exit(1);
    }

    return (long long)now.tv_sec * MS_PER_SECOND
         + now.tv_nsec / NS_PER_MS;
}

void Rotary_init(int buttonLine){

    // open the gpio chip used by the rotary encoder
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (chip == NULL){
        perror("Rotary GPIO chip");
        exit(1);
    }

    // get the gpio lines connected to channels A and B
    lineA = gpiod_chip_get_line(chip, GPIO_A);
    lineB = gpiod_chip_get_line(chip, GPIO_B);

    if (lineA == NULL || lineB == NULL){
        perror("Rotary GPIO lines");
        exit(1);
    }

    // configure both channels as inputs with internal pull-up resistors
    // the inputs are high when the encoder contacts are open
    if (gpiod_line_request_input_flags(lineA, "rotary-A",
            GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0){
        perror("Rotary GPIO A");
        exit(1);
    }

    if (gpiod_line_request_input_flags(lineB, "rotary-B",
            GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0){
        perror("Rotary GPIO B");
        exit(1);
    }

    // read the initial values of channels A and B
    int a = gpiod_line_get_value(lineA);
    int b = gpiod_line_get_value(lineB);

    if (a < 0 || b < 0){
        perror("Rotary initial read");
        exit(1);
    }

    // combine A and B into a 2-bit state
    // A is the higher bit and B is the lower bit
    previousState = (a << CHANNEL_A_SHIFT) | b;
    steps = 0;

    // initialize the optional push button if a gpio line is provided
    // passing -1 skips the button so rotation can be tested separately
    if (buttonLine >= 0){

        // make sure the button is not using an encoder channel line
        if (buttonLine == GPIO_A || buttonLine == GPIO_B){
            fprintf(stderr, "Rotary SW cannot share GPIO A or B\n");
            exit(1);
        }

        // request the button as an input with a pull-up resistor
        lineSW = gpiod_chip_get_line(chip, buttonLine);
        if (lineSW == NULL || gpiod_line_request_input_flags(lineSW,
                "rotary-SW", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0){
            perror("Rotary GPIO SW");
            exit(1);
        }

        // store the starting button state for debounce detection
        int button = gpiod_line_get_value(lineSW);
        if (button < 0){
            perror("Rotary button initial read");
            exit(1);
        }

        lastRawButton = button;
        stableButton = button;
        lastButtonChangeMs = getTimeMs();
        pressStartMs = 0;
    }
}

int Rotary_getDirection(void){

    // return if the encoder has not been initialized
    if (lineA == NULL || lineB == NULL){
        return DIRECTION_NONE;
    }

    // read the current values of both encoder channels
    int a = gpiod_line_get_value(lineA);
    int b = gpiod_line_get_value(lineB);

    if (a < 0 || b < 0){
        return DIRECTION_NONE;
    }

    // combine the channel readings into a 2-bit state
    int currentState = (a << CHANNEL_A_SHIFT) | b;

    // no movement is detected if the state has not changed
    if (currentState == previousState){
        return DIRECTION_NONE;
    }

    // combine the previous and current states into a 4-bit transition
    // this is the same quadrature detection method used in assignment 2
    int transition = (previousState << PREVIOUS_STATE_SHIFT)
                   | currentState;
    previousState = currentState;

    // count valid clockwise transitions
    if (transition == CW_11_TO_10 ||
        transition == CW_10_TO_00 ||
        transition == CW_00_TO_01 ||
        transition == CW_01_TO_11){
        steps++;
    }

    // count valid counter-clockwise transitions
    else if (transition == CCW_11_TO_01 ||
             transition == CCW_01_TO_00 ||
             transition == CCW_00_TO_10 ||
             transition == CCW_10_TO_11){
        steps--;
    }
    else{
        // reset the counter if an invalid transition is detected
        steps = 0;
    }

    // register a full step when both channels return high (11)
    // this prevents counting each individual transition as a step
    if (currentState == ROTARY_REST_STATE){
        int direction = DIRECTION_NONE;

        // count a clockwise step when enough valid transitions occur
        if (steps >= STEP_THRESHOLD){
            direction = DIRECTION_CW;
        }

        // count a counter-clockwise step when enough transitions occur
        else if (steps <= -STEP_THRESHOLD){
            direction = DIRECTION_CCW;
        }

        // reset the counter for the next encoder step
        steps = 0;
        return direction;
    }

    // return no direction until a complete step has been detected
    return DIRECTION_NONE;
}

int Rotary_getButtonEvent(void){

    // return no event if the optional button is not connected
    if (lineSW == NULL){
        return ROTARY_BUTTON_NONE;
    }

    // read the button state and get the current time
    int button = gpiod_line_get_value(lineSW);
    if (button < 0){
        return ROTARY_BUTTON_NONE;
    }

    long long now = getTimeMs();

    // record when the raw button reading changes
    // this helps ignore brief changes caused by button bouncing
    if (button != lastRawButton){
        lastRawButton = button;
        lastButtonChangeMs = now;
    }

    // only accept a change after the button is stable for 40 ms
    if (button != stableButton &&
        now - lastButtonChangeMs >= BUTTON_DEBOUNCE_MS){

        stableButton = button;

        // record the time when the button is pressed (active-low)
        if (stableButton == BUTTON_PRESSED){
            pressStartMs = now;
        }

        // when released, calculate how long the button was held
        else if (stableButton == BUTTON_RELEASED && pressStartMs > 0){
            long long heldMs = now - pressStartMs;
            pressStartMs = 0;

            // return a long press if the button was held long enough
            if (heldMs >= LONG_PRESS_MS){
                return ROTARY_BUTTON_LONG;
            }

            // otherwise return one short-press event
            return ROTARY_BUTTON_SHORT;
        }
    }

    // return no event while the button is idle or still being held
    return ROTARY_BUTTON_NONE;
}

void Rotary_cleanup(void){

    // release the optional push button gpio line
    if (lineSW != NULL){
        gpiod_line_release(lineSW);
    }

    // release the gpio lines used by the rotary encoder
    if (lineA != NULL){
        gpiod_line_release(lineA);
    }
    if (lineB != NULL){
        gpiod_line_release(lineB);
    }

    // close the gpio chip after releasing its lines
    if (chip != NULL){
        gpiod_chip_close(chip);
    }

    // clear stored pointers after releasing resources
    lineSW = NULL;
    lineA = NULL;
    lineB = NULL;
    chip = NULL;
}
