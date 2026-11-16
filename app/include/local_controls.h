/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
Application module: Local controls

handles joystick and rotary encoder input for
local parking garage administration.
*/

#ifndef LOCAL_CONTROLS_H
#define LOCAL_CONTROLS_H

// initialize the joystick, rotary encoder and local controls
void LocalControls_init(void);

// read inputs and process local control actions
void LocalControls_update(void);

// release joystick and rotary encoder resources
void LocalControls_cleanup(void);

#endif
