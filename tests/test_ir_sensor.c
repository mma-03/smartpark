
/*
ENSC 351 - Design Project - SmartPark
Author: Laxmi Unnithan

tests the entrance and exit IR sensors by displaying car detection
events and final detection counts.
*/

#define _POSIX_C_SOURCE 200809L

#include "hal/ir_sensor.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// GPIO connections for entrance and exit sensors
#define ENTRANCE_CHIP "/dev/gpiochip2"
#define ENTRANCE_LINE 8

#define EXIT_CHIP "/dev/gpiochip1"
#define EXIT_LINE 33

// KY-033 outputs LOW when an object is detected
#define IR_ACTIVE_LEVEL 0

// check the sensors every 20 ms
#define POLL_DELAY_NS 20000000L

// allow Ctrl+C to stop the test
static volatile sig_atomic_t running = 1;

static void stopProgram(int signalNumber)
{
    (void)signalNumber;
    running = 0;
}

// check for new detections and when a car leaves
static int testSensor(IrSensor *sensor, const char *name)
{
    int reading = IrSensor_gpioRead(sensor);

    if (reading < 0) {
        fprintf(stderr, "%s GPIO read failed\n", name);
        return -1;
    }

    int wasPresent = IrSensor_isPresent(sensor);
    int event = IrSensor_update(sensor, reading);
    int isPresent = IrSensor_isPresent(sensor);

    if (event < 0) {
        fprintf(stderr, "%s sensor update failed\n", name);
        return -1;
    }

    if (event == 1) {
        printf("%s: Car detected! Count = %d\n",
               name, IrSensor_getCount(sensor));
    }
    else if (wasPresent && !isPresent) {
        printf("%s: Car left sensor\n", name);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int useEntrance = 1;
    int useExit = 1;

    // select which sensors to test
    if (argc == 2) {
        if (strcmp(argv[1], "--entrance") == 0) {
            useExit = 0;
        }
        else if (strcmp(argv[1], "--exit") == 0) {
            useEntrance = 0;
        }
        else if (strcmp(argv[1], "--both") != 0) {
            fprintf(stderr,
                    "Usage: %s [--entrance|--exit|--both]\n",
                    argv[0]);
            return 1;
        }
    }
    else if (argc != 1) {
        fprintf(stderr,
                "Usage: %s [--entrance|--exit|--both]\n",
                argv[0]);
        return 1;
    }

    signal(SIGINT, stopProgram);
    signal(SIGTERM, stopProgram);

    IrSensor entrance;
    IrSensor exitSensor;

    IrSensor_init(&entrance, IR_ACTIVE_LEVEL);
    IrSensor_init(&exitSensor, IR_ACTIVE_LEVEL);

    // initialize entrance GPIO
    if (useEntrance) {
        if (IrSensor_gpioInit(&entrance,
                              ENTRANCE_CHIP,
                              ENTRANCE_LINE) != 0) {
            fprintf(stderr, "entrance sensor initialization failed\n");
            return 1;
        }
    }

    // initialize exit GPIO
    if (useExit) {
        if (IrSensor_gpioInit(&exitSensor,
                              EXIT_CHIP,
                              EXIT_LINE) != 0) {
            fprintf(stderr, "exit sensor initialization failed\n");

            if (useEntrance) {
                IrSensor_gpioCleanup(&entrance);
            }

            return 1;
        }
    }

    printf("SmartPark IR Sensor Test\n");
    printf("Entrance: %s\n", useEntrance ? "enabled" : "disabled");
    printf("Exit: %s\n", useExit ? "enabled" : "disabled");
    printf("Press Ctrl+C to exit\n\n");

    struct timespec delay = {0, POLL_DELAY_NS};
    int result = 0;

    while (running) {
        if (useEntrance) {
            if (testSensor(&entrance, "ENTRANCE") < 0) {
                result = 1;
                break;
            }
        }

        if (useExit) {
            if (testSensor(&exitSensor, "EXIT") < 0) {
                result = 1;
                break;
            }
        }

        nanosleep(&delay, NULL);
    }

    // release GPIO connections
    if (useEntrance) {
        IrSensor_gpioCleanup(&entrance);
    }

    if (useExit) {
        IrSensor_gpioCleanup(&exitSensor);
    }

    printf("\nFinal entrance count: %d\n",
           IrSensor_getCount(&entrance));
    printf("Final exit count: %d\n",
           IrSensor_getCount(&exitSensor));

    return result;
}
