/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara

tests both local control inputs without operating the garage hardware.
*/

#define _POSIX_C_SOURCE 200809L

#include "local_controls.h"
#include <signal.h>
#include <stdio.h>
#include <time.h>

// the 0.2 ms polling interval needed by the AS2 rotary encoder
#define POLL_DELAY_NS 200000L

// allow ctrl+c to stop the test
static volatile sig_atomic_t running = 1;

static void stopProgram(int signalNumber){
    (void)signalNumber;
    running = 0;
}

int main(void){
    signal(SIGINT, stopProgram);

    // initialize both local input devices
    LocalControls_init();
    printf("press Ctrl+C to exit\n");

    struct timespec delay = {0, POLL_DELAY_NS};

    while (running){
        LocalControls_update();
        nanosleep(&delay, NULL);
    }

    // release gpio and spi resources
    LocalControls_cleanup();
    return 0;
}
