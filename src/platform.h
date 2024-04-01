//
// Created by WangKZ on 2024/4/1.
//

#ifndef WINES_PLATFORM_H
#define WINES_PLATFORM_H

#include "common.h"


typedef struct file {

    size_t (* read)(struct file* f, void* out_data, size_t size);

    size_t (* write)(struct file* f, const void* in_data, size_t size);

    void (* seek)(struct file* f, long offset, int whence);

    void (* close)(struct file* f);

    void* handle;
} file_t;

file_t* open_file(const char* filename, const char* mode);

bool file_exists(const char* filename);

#endif //WINES_PLATFORM_H
