/*
ENSC 351 - Design Project - SmartPark
Author: Anushka Kulasekara
HAL module: Joystick

based on the joystick HAL from Assignment 1.
communicates with the MCP3208 ADC using SPI and detects joystick direction.
*/

#include "hal/joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// spi device and settings used with the MCP3208
#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED_HZ 250000
#define SPI_BITS_PER_WORD 8
#define SPI_TRANSFER_BYTES 3

// joystick channels and direction thresholds from Assignment 1
#define JOYSTICK_X_CHANNEL 0
#define JOYSTICK_Y_CHANNEL 1
#define JOYSTICK_LOW_THRESHOLD 600
#define JOYSTICK_HIGH_THRESHOLD 3500

// MCP3208 command and response bit values
#define ADC_MAX_CHANNEL 7
#define ADC_SINGLE_ENDED_COMMAND 0x06
#define ADC_CHANNEL_HIGH_SHIFT 2
#define ADC_CHANNEL_LOW_MASK 0x03
#define ADC_CHANNEL_LOW_SHIFT 6
#define ADC_RESULT_HIGH_MASK 0x0F
#define ADC_RESULT_HIGH_SHIFT 8

// keep track of the spi file descriptor
static int spiFd = -1;

void Joystick_init(void){

    // open the spi device used by the MCP3208
    spiFd = open(SPI_DEVICE, O_RDWR);
    if (spiFd < 0){
        perror("Joystick SPI open");
        exit(1);
    }

    // set spi mode 0, the same mode used in Assignment 1
    uint8_t mode = SPI_MODE_0;
    if (ioctl(spiFd, SPI_IOC_WR_MODE, &mode) < 0){
        perror("Joystick SPI mode");
        Joystick_cleanup();
        exit(1);
    }
}

int Joystick_read(int channel){

    // reject invalid channels or reads before initialization
    if (spiFd < 0 || channel < 0 || channel > ADC_MAX_CHANNEL){
        return -1;
    }

    // send a three-byte command to select the MCP3208 channel
    uint8_t tx[SPI_TRANSFER_BYTES] = {
        ADC_SINGLE_ENDED_COMMAND | (channel >> ADC_CHANNEL_HIGH_SHIFT),
        (channel & ADC_CHANNEL_LOW_MASK) << ADC_CHANNEL_LOW_SHIFT,
        0
    };
    uint8_t rx[SPI_TRANSFER_BYTES] = {0};

    // transfer the command and receive the ADC result
    struct spi_ioc_transfer transfer = {
        .tx_buf = (uintptr_t)tx,
        .rx_buf = (uintptr_t)rx,
        .len = SPI_TRANSFER_BYTES,
        .speed_hz = SPI_SPEED_HZ,
        .bits_per_word = SPI_BITS_PER_WORD
    };

    if (ioctl(spiFd, SPI_IOC_MESSAGE(1), &transfer) < 1){
        perror("Joystick SPI read");
        return -1;
    }

    // combine the returned bytes into a 12-bit ADC reading
    return ((rx[1] & ADC_RESULT_HIGH_MASK) << ADC_RESULT_HIGH_SHIFT) | rx[2];
}

JoystickDirection Joystick_getDirection(void){

    // read the x and y joystick axes
    int x = Joystick_read(JOYSTICK_X_CHANNEL);
    int y = Joystick_read(JOYSTICK_Y_CHANNEL);

    // return no movement if an ADC read failed
    if (x < 0 || y < 0){
        return JOYSTICK_NONE;
    }

    // check vertical movement first, then horizontal movement
    if (y > JOYSTICK_HIGH_THRESHOLD){
        return JOYSTICK_UP;
    }
    if (y < JOYSTICK_LOW_THRESHOLD){
        return JOYSTICK_DOWN;
    }
    if (x > JOYSTICK_HIGH_THRESHOLD){
        return JOYSTICK_RIGHT;
    }
    if (x < JOYSTICK_LOW_THRESHOLD){
        return JOYSTICK_LEFT;
    }

    // return no movement while the joystick is near the center
    return JOYSTICK_NONE;
}

void Joystick_cleanup(void){

    // close the spi device if it was opened
    if (spiFd >= 0){
        close(spiFd);
        spiFd = -1;
    }
}
