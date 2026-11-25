#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>

#define FRAME_WIDTH_PX 176
#define FRAME_HEIGHT_PX 144
#define FRAME_PIXELS (FRAME_WIDTH_PX * FRAME_HEIGHT_PX)

typedef struct {
    int width;
    int height;
    uint8_t *pixels;
} Frame;

#endif