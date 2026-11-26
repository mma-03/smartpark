/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
HAL test: Servo

tests the MG90S servo motor independently before integrating
the gate control module with the remaining SmartPark modules.
*/

#include "hal/servo.h"
#include <stdio.h>

// keyboard commands used to test the servo
#define OPEN_COMMAND 'o'
#define CLOSE_COMMAND 'c'
#define QUIT_COMMAND 'q'

int main(void){

    // initialize the servo pwm interface
    if (Servo_init() < 0){
        printf("Servo initialization failed.\n");
        return 1;
    }

    printf("Servo initialized successfully.\n");
    printf("Servo Test\n");
    printf("o - Open gate\n");
    printf("c - Close gate\n");
    printf("q - Quit\n");

    // read keyboard commands to test each servo position
    int command;

    while ((command = getchar()) != EOF){

        if (command == OPEN_COMMAND){

            // request the open gate position
            if (Servo_open() == 0){
                printf("Opening gate.\n");
            }
            else{
                printf("Failed to open gate.\n");
            }
        }
        else if (command == CLOSE_COMMAND){

            // request the closed gate position
            if (Servo_close() == 0){
                printf("Closing gate.\n");
            }
            else{
                printf("Failed to close gate.\n");
            }
        }
        else if (command == QUIT_COMMAND){

            // exit the test loop
            break;
        }
    }

    // disable pwm before exiting the program
    Servo_cleanup();

    printf("Servo test ended.\n");

    return 0;
}
