//
// Created by WangKZ on 2024/4/1.
//

#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)

#include <io.h>

#define ACCESS _access
#define F_OK 0

#else

#include <unistd.h>
#define ACCESS access

#endif


static size_t file_read(file_t* file, void* out_data, size_t size) {
    return fread(out_data, 1, size, file->handle);
}

static size_t file_write(file_t* file, const void* in_data, size_t size) {
    return fwrite(in_data, 1, size, file->handle);
}

static void file_seek(file_t* file, long offset, int whence) {
    fseek(file->handle, offset, whence);
}

static void file_close(file_t* file) {
    fclose(file->handle);
    free(file);
}

file_t* open_file(const char* filename, const char* mode) {
    FILE* file;
#if defined(_WIN32) || defined(_WIN64)
    fopen_s(&file, filename, mode);
#else
    file = fopen(filename, mode);
#endif
    file_t* ret = malloc(sizeof(file_t));

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