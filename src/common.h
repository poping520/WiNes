//
// Created by WangKZ on 2024/3/29.
//

#ifndef WINES_COMMON_H
#define WINES_COMMON_H

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t addr_t;

typedef int err_t;

#define ERR_OK              0
#define ERR_NULLPTR         1
#define ERR_FILE_NOT_EXISTS 2
#define ERR_INVALID_ROM     3
#define ERR_NES_FORMAT      4


void* wn_malloc(size_t size);

void* wn_calloc(size_t size);

void wn_free(void* ptr);


#endif //WINES_COMMON_H
