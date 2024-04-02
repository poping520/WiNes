//
// Created by WangKZ on 2024/4/1.
//

#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)

#include <Windows.h>
#include <io.h>

#define ACCESS _access
#define F_OK 0
#define MSLEEP(MS) Sleep(MS)

#else

#include <unistd.h>

#define MSLEEP(MS) usleep(MS*1000)
#define ACCESS access

#endif


static size_t file_read(wn_file_t* this, void* out_data, size_t size) {
    return fread(out_data, 1, size, this->handle);
}

static size_t file_write(wn_file_t* this, const void* in_data, size_t size) {
    return fwrite(in_data, 1, size, this->handle);
}

static void file_seek(wn_file_t* this, long offset, int whence) {
    fseek(this->handle, offset, whence);
}

static void file_close(wn_file_t* this) {
    fclose(this->handle);
    free(this);
}

wn_file_t* open_file(const char* filename, const char* mode) {
    FILE* file;
#if defined(_WIN32) || defined(_WIN64)
    fopen_s(&file, filename, mode);
#else
    file = fopen(filename, mode);
#endif
    wn_file_t* ret = malloc(sizeof(wn_file_t));

    ret->handle = file;
    ret->read = file_read;
    ret->write = file_write;
    ret->seek = file_seek;
    ret->close = file_close;
    return ret;
}


bool file_exists(const char* filename) {
    if (filename == NULL) {
        return false;
    }
    return ACCESS(filename, F_OK) != -1;
}

void wn_msleep(long millisecond) {
    MSLEEP(millisecond);
}