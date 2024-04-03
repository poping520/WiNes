//
// Created by WangKZ on 2024/4/1.
//

#ifndef WINES_PLATFORM_H
#define WINES_PLATFORM_H

#include "common.h"


typedef struct wn_file wn_file_t;

struct wn_file {

    size_t (* read)(wn_file_t* this, void* out_data, size_t size);

    size_t (* write)(wn_file_t* this, const void* in_data, size_t size);

    void (* seek)(wn_file_t* this, long offset, int whence);

    void (* close)(wn_file_t* this);

    void* handle;
};

wn_file_t* open_file(const char* filename, const char* mode);

bool file_exists(const char* filename);

void wn_msleep(long millisecond);

void wn_nano_sleep(long nanosecond);

#endif //WINES_PLATFORM_H
