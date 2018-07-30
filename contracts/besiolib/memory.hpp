/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#pragma once

#include <besiolib/memory.h>
#include <besiolib/print.hpp>

void* sbrk(size_t num_bytes);

extern "C" {

void* malloc(size_t size);

void* calloc(size_t count, size_t size);

void* realloc(void* ptr, size_t size);

void free(void* ptr);

}
