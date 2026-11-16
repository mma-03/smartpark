/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
Application module: Local controls

reads the joystick and rotary encoder to request gate actions,
select parking spaces, and manage local administrator controls.
for now, all actions are printed for testing only.
*/

#include "local_controls.h"
#include "hal/joystick.h"
#include "hal/rotary.h"
#include <stdio.h>

// rotary push button uses GPIO18 (gpiochip2 line 11)
#define ROTARY_BUTTON_LINE 11

// use four parking spaces for the initial test
#define NUM_PARKING_SPACES 4

// check joystick every 250 updates (about 50 ms at 0.2 ms per update)
#define JOYSTICK_POLL_COUNT 250

// rotary direction values returned by the HAL
#define ROTARY_CLOCKWISE 1
#define ROTARY_COUNTERCLOCKWISE -1

// local state used until the shared garage state is integrated
static int selectedSpace = 0;
static int blockedSpaces[NUM_PARKING_SPACES] = {0};
static int manualMode = 0;
static int joystickCount = 0;
static JoystickDirection previousJoystick = JOYSTICK_NONE;

void LocalControls_init(void){

    // initialize the hardware modules
    Joystick_init();
    Rotary_init(ROTARY_BUTTON_LINE);

    // initialize the local garage settings
    selectedSpace = 0;
    manualMode = 0;
    joystickCount = 0;
    previousJoystick = Joystick_getDirection();

    for (int i = 0; i < NUM_PARKING_SPACES; i++){
        blockedSpaces[i] = 0;
    }

    printf("SmartPark Local Controls Test\n");
    printf("automatic mode enabled\n");
    printf("selected parking space: 1\n");
}

void LocalControls_update(void){

    // check rotary rotation on every update to avoid missing steps
    int rotation = Rotary_getDirection();

    if (rotation == ROTARY_CLOCKWISE){
        // move to the next parking space and wrap to the first
        selectedSpace = (selectedSpace + 1) % NUM_PARKING_SPACES;
        printf("rotary CW: selected parking space %d\n", selectedSpace + 1);
    }
    else if (rotation == ROTARY_COUNTERCLOCKWISE){
        // move to the previous parking space and wrap to the last
        selectedSpace = (selectedSpace + NUM_PARKING_SPACES - 1)
                        % NUM_PARKING_SPACES;
        printf("rotary CCW: selected parking space %d\n", selectedSpace + 1);
    }

    // check the push button for short and long press events
    int button = Rotary_getButtonEvent();

    if (button == ROTARY_BUTTON_SHORT){
        // update the temporary administrative blocked state
        blockedSpaces[selectedSpace] = !blockedSpaces[selectedSpace];
        printf("rotary short press: space %d %s\n",
               selectedSpace + 1,
               blockedSpaces[selectedSpace] ? "blocked" : "unblocked");
    }
    else if (button == ROTARY_BUTTON_LONG){
        // only request calibration; the vision module is not connected yet
        printf("rotary long press: camera calibration requested\n");
        printf("confirm the garage is empty before calibration\n");
    }

    // read joystick less often than the rotary encoder
    joystickCount++;
    if (joystickCount < JOYSTICK_POLL_COUNT){
        return;
    }
    joystickCount = 0;

    JoystickDirection direction = Joystick_getDirection();

    // only process a joystick action when its direction changes
    if (direction == previousJoystick){
        return;
    }
    previousJoystick = direction;

    if (direction == JOYSTICK_RIGHT){
        manualMode = 1;
        printf("joystick RIGHT: manual mode enabled\n");
    }
    else if (direction == JOYSTICK_LEFT){
        manualMode = 0;
        printf("joystick LEFT: automatic mode enabled\n");
    }
    else if (direction == JOYSTICK_UP){
        if (manualMode){
            printf("joystick UP: gate open requested\n");
        }
        else{
            printf("joystick UP: ignored in automatic mode\n");
        }
    }
    else if (direction == JOYSTICK_DOWN){
        if (manualMode){
            printf("joystick DOWN: gate close requested\n");
        }
        else{
            printf("joystick DOWN: ignored in automatic mode\n");
        }
    }
}

void LocalControls_cleanup(void){

    // release the rotary encoder and joystick resources
    Rotary_cleanup();
    Joystick_cleanup();
}
