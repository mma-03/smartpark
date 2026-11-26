#include "pgm.h"
#include <stdio.h>
#include <string.h>

int pgm_write(const char *path, const Frame *frame){
    if (path == NULL || frame == NULL || frame->pixels == NULL || frame->width <= 0 || frame->height <= 0) {
        return -1;
    }

    FILE *pgm_file = fopen(path, "wb");
    if (pgm_file == NULL) {
        perror("pgm_write: opening file");
        return -1;
    }

    // write the header:
    // P5
    // <width> <height>
    // 255
    if (fprintf(pgm_file, "P5\n%d %d\n255\n", frame->width, frame->height) < 0) {
        perror("pgm_write: header");
        fclose(pgm_file);
        return -1;
    }

    size_t pixel_count = frame->width * frame->height;
    size_t pixels_written = fwrite(frame->pixels, 1, pixel_count, pgm_file);
    if (pixels_written != pixel_count) {
        perror("pgm_write: write pixels");
        fclose(pgm_file);
        return -1;
    }

    if (fclose(pgm_file) != 0) {
        perror("pgm_write: closing file");
        return -1;
    }

    return 0;
}

int pgm_read(const char *path, Frame *out){
    if (path == NULL || out == NULL || out->pixels == NULL) {
        return -1;
    }

    FILE *pgm_file = fopen(path, "rb");
    if (pgm_file == NULL) {
        perror("pgm_read: opening file");
        return -1;
    }

    char pgm_check[3];
    if (fscanf(pgm_file, "%2s", pgm_check) != 1){
        fprintf(stderr, "pgm_read: could not read header\n");
        fclose(pgm_file);
        return -1;
    }
    if (strcmp(pgm_check, "P5") != 0) {
        fprintf(stderr, "pgm_read: not a P5 PGM\n");
        fclose(pgm_file);
        return -1;
    }
    
    int file_width;
    int file_height;
    int max_value;

    if (fscanf(pgm_file, "%d %d %d", &file_width, &file_height, &max_value) != 3) {
        fprintf(stderr, "pgm_read: broken header\n");
        fclose(pgm_file);
        return -1;
    }
    if (file_width <= 0 || file_height <= 0) {
        fprintf(stderr, "pgm_read: invalid size\n");
        fclose(pgm_file);
        return -1;
    }
    if (max_value != 255) {
        fprintf(stderr, "pgm_read: unsupported max value\n");
        fclose(pgm_file);
        return -1;
    }
    if (file_width * file_height > FRAME_WIDTH_PX * FRAME_HEIGHT_PX) {
        fprintf(stderr, "pgm_read: image too large\n");
        fclose(pgm_file);
        return -1;
    }

    fgetc(pgm_file);

    size_t pixel_count = file_width * file_height;
    size_t pixels_read = fread(out->pixels, 1, pixel_count, pgm_file);
    if (pixels_read != pixel_count) {
        fprintf(stderr, "pgm_read: file too short\n");
        fclose(pgm_file);
        return -1;
    }

    out->width = file_width;
    out->height = file_height;
    fclose(pgm_file);
    return 0;
}