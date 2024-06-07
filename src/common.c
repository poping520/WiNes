//
// Created by WangKZ on 2024/4/1.
//

#include "common.h"

void* wn_malloc(size_t size) {
    return malloc(size);
}

void* wn_calloc(size_t size) {
    return calloc(1, size);
}

void wn_free(void* ptr) {
    if (ptr != NULL) {
        free(ptr);
        ptr = NULL;
    }
}