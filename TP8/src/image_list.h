#ifndef IMAGE_LIST_H
#define IMAGE_LIST_H

#include "image_data.h"
#include <stdint.h>

const int8_t *const image_list[10] = {
    image_0_data,
    image_1_data,
    image_2_data,
    image_3_data,
    image_4_data,
    image_5_data,
    image_6_data,
    image_7_data,
    image_8_data,
    image_9_data,
};

const int NUM_IMAGES = sizeof(image_list) / sizeof(image_list[0]);

#endif // IMAGE_LIST_H
