/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
HAL module: Rotary encoder

reads the rotary encoder connected to the BeagleY-AI using libgpiod.
detects clockwise and counter-clockwise rotation and button presses.

*reused code from Assignment 2
*updated to use libgpiod 2.x

*/

#define _POSIX_C_SOURCE 200809L

#include "hal/rotary.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// gpio chip and line numbers used for encoder channels A and B
#define GPIO_CHIP "/dev/gpiochip2"
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

// store the gpio chip and line requests for the rotary encoder
static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *requestA = NULL;
static struct gpiod_line_request *requestB = NULL;
static struct gpiod_line_request *requestSW = NULL;

// store the gpio line number used by the optional push button
static unsigned int buttonOffset = 0;

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

// configure and request a gpio line as an input with a pull-up resistor
static struct gpiod_line_request *requestInput(
        unsigned int offset, const char *consumer){

    // create the gpio settings and configuration objects
    struct gpiod_line_settings *settings =
        gpiod_line_settings_new();

    struct gpiod_line_config *config =
        gpiod_line_config_new();

    struct gpiod_request_config *requestConfig =
        gpiod_request_config_new();

    struct gpiod_line_request *request = NULL;

    // check that all configuration objects were created
    if (settings == NULL || config == NULL ||
        requestConfig == NULL){
        fprintf(stderr, "Rotary GPIO configuration failed\n");
    }
    else{

        // configure the gpio line as an input with a pull-up resistor
        // keep the default polarity so 0 is low and 1 is high
        if (gpiod_line_settings_set_direction(settings,
                GPIOD_LINE_DIRECTION_INPUT) < 0 ||
            gpiod_line_settings_set_bias(settings,
                GPIOD_LINE_BIAS_PULL_UP) < 0){
            perror("Rotary GPIO settings");
        }

        // apply the settings to the requested gpio line
        else if (gpiod_line_config_add_line_settings(
                config, &offset, 1, settings) < 0){
            perror("Rotary GPIO line configuration");
        }
        else{

            // assign a name to identify the gpio line request
            gpiod_request_config_set_consumer(
                requestConfig, consumer);

            // request access to the configured gpio line
            request = gpiod_chip_request_lines(
                chip, requestConfig, config);

            if (request == NULL){
                perror("Rotary GPIO request");
            }
        }
    }

    // free the configuration objects after requesting the gpio line
    // the line request remains available for reading
    if (settings != NULL){
        gpiod_line_settings_free(settings);
    }

    if (config != NULL){
        gpiod_line_config_free(config);
    }

    if (requestConfig != NULL){
        gpiod_request_config_free(requestConfig);
    }

    return request;
}

void Rotary_init(int buttonLine){

    // open the gpio chip used by the rotary encoder
    chip = gpiod_chip_open(GPIO_CHIP);
    if (chip == NULL){
        perror("Rotary GPIO chip");
        exit(1);
    }

    // request channels A and B as inputs with pull-up resistors
    // the inputs are high when the encoder contacts are open
    requestA = requestInput(GPIO_A, "rotary-A");
    requestB = requestInput(GPIO_B, "rotary-B");

    if (requestA == NULL || requestB == NULL){
        fprintf(stderr, "Rotary GPIO lines failed\n");
        Rotary_cleanup();
        exit(1);
    }

    // read the initial values of channels A and B
    int a = gpiod_line_request_get_value(requestA, GPIO_A);
    int b = gpiod_line_request_get_value(requestB, GPIO_B);

    if (a < 0 || b < 0){
        perror("Rotary initial read");
        Rotary_cleanup();
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
            Rotary_cleanup();
            exit(1);
        }

        // store the button offset for future gpio reads
        buttonOffset = (unsigned int)buttonLine;

        // request the button as an input with a pull-up resistor
        requestSW = requestInput(buttonOffset, "rotary-SW");

        if (requestSW == NULL){
            fprintf(stderr, "Rotary GPIO SW failed\n");
            Rotary_cleanup();
            exit(1);
        }

        // store the starting button state for debounce detection
        int button = gpiod_line_request_get_value(
            requestSW, buttonOffset);

        if (button < 0){
            perror("Rotary button initial read");
            Rotary_cleanup();
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
    if (requestA == NULL || requestB == NULL){
        return DIRECTION_NONE;
    }

    // read the current values of both encoder channels
    int a = gpiod_line_request_get_value(requestA, GPIO_A);
    int b = gpiod_line_request_get_value(requestB, GPIO_B);

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
    if (requestSW == NULL){
        return ROTARY_BUTTON_NONE;
    }

    // read the button state and get the current time
    int button = gpiod_line_request_get_value(
        requestSW, buttonOffset);

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

    // release the optional push button gpio request
    if (requestSW != NULL){
        gpiod_line_request_release(requestSW);
    }

    // release the gpio requests used by the rotary encoder
    if (requestA != NULL){
        gpiod_line_request_release(requestA);
    }

    if (requestB != NULL){
        gpiod_line_request_release(requestB);
    }

    // close the gpio chip after releasing its requests
    if (chip != NULL){
        gpiod_chip_close(chip);
    }

    // clear stored pointers after releasing resources
    requestSW = NULL;
    requestA = NULL;
    requestB = NULL;
    chip = NULL;
    buttonOffset = 0;
}
