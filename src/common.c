//
// Created by WangKZ on 2024/4/1.
//

#include <malloc.h>

void* pn_malloc(size_t size) {
    return malloc(size);
}

void pn_free(void* ptr) {
    if (ptr != NULL) {
        free(ptr);
        ptr = NULL;
    }
}