#ifndef PGM_H
#define PGM_H

#include "hal/frame.h"

// returns 0/-1, writes the PGM header then the pixels to the file at 'path'
int pgm_write(const char *path, const Frame *frame);
// returns 0/-1, reads the file at 'path' into 'out'
int pgm_read(const char *path, Frame *out);

#endif