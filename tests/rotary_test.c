/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara

tests the rotary encoder by printing clockwise and
counter-clockwise rotation and button events.
*/

#define _POSIX_C_SOURCE 200809L

#include "hal/rotary.h"
#include <stdio.h>
#include <signal.h>
#include <time.h>

// gpio line connected to the rotary encoder push button
#define GPIO_SW 11

// flag used to stop the program when ctrl+c is pressed
static volatile sig_atomic_t running = 1;

// stop the program when ctrl+c is pressed
static void stopProgram(int signalNumber){
    (void)signalNumber;
    running = 0;
}

int main(void){

    // allow the program to exit using ctrl+c
    signal(SIGINT, stopProgram);

    // initialize the rotary encoder and push button
    Rotary_init(GPIO_SW);

    printf("SmartPark Rotary Test\n");
    printf("Rotate encoder to test direction\n");
    printf("Press Ctrl+C to exit\n");

    // delay of 0.2 ms between readings
    struct timespec delay = {0, 200000};

    while (running){

        // read the direction of the rotary encoder
        int direction = Rotary_getDirection();

        if (direction == 1){
            printf("Clockwise\n");
        }
        else if (direction == -1){
            printf("Counter-clockwise\n");
        }

        // read button events if the button is connected
        int button = Rotary_getButtonEvent();

        if (button == ROTARY_BUTTON_SHORT){
            printf("Short button press\n");
        }
        else if (button == ROTARY_BUTTON_LONG){
            printf("Long button press\n");
        }

        // wait before reading the encoder again
        nanosleep(&delay, NULL);
    }

    // release gpio resources
    Rotary_cleanup();

    return 0;
}
