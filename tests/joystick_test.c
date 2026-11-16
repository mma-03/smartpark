/*
ENSC 351 - Design Project -SmartPark
Author: Anushka Kulasekara

tests the joystick by printing ADC readings and direction changes.
*/

#define _POSIX_C_SOURCE 200809L

#include "hal/joystick.h"
#include <signal.h>
#include <stdio.h>
#include <time.h>

// joystick channels used in assignment 1
#define JOYSTICK_X_CHANNEL 0
#define JOYSTICK_Y_CHANNEL 1

// check the joystick every 50 ms and print ADC values every 500 ms
#define POLL_DELAY_NS 50000000L
#define READINGS_PER_PRINT 10

// allow ctrl+c to stop the test
static volatile sig_atomic_t running = 1;

static void stopProgram(int signalNumber){
    (void)signalNumber;
    running = 0;
}

int main(void){
    signal(SIGINT, stopProgram);

    // initialize the joystick and MCP3208 connection
    Joystick_init();

    printf("SmartPark Joystick Test\n");
    printf("Move the joystick UP, DOWN, LEFT, and RIGHT\n");
    printf("Press Ctrl+C to exit\n");

    JoystickDirection previousDirection = JOYSTICK_NONE;
    struct timespec delay = {0, POLL_DELAY_NS};
    int printCount = 0;

    while (running){
        // read both joystick channels to check the raw ADC values
        int x = Joystick_read(JOYSTICK_X_CHANNEL);
        int y = Joystick_read(JOYSTICK_Y_CHANNEL);

        if (x < 0 || y < 0){
            fprintf(stderr, "joystick ADC read failed\n");
            break;
        }

        // print the ADC values twice per second for debugging
        if (printCount == 0){
            printf("X=%d Y=%d\n", x, y);
        }
        printCount = (printCount + 1) % READINGS_PER_PRINT;

        // report only when the joystick direction changes
        JoystickDirection direction = Joystick_getDirection();

        if (direction != previousDirection){
            if (direction == JOYSTICK_UP){
                printf("UP\n");
            }
            else if (direction == JOYSTICK_DOWN){
                printf("DOWN\n");
            }
            else if (direction == JOYSTICK_LEFT){
                printf("LEFT\n");
            }
            else if (direction == JOYSTICK_RIGHT){
                printf("RIGHT\n");
            }
            else{
                printf("CENTER\n");
            }

            previousDirection = direction;
        }

        nanosleep(&delay, NULL);
    }

    // close the SPI connection before exiting
    Joystick_cleanup();
    return 0;
}
