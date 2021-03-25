#ifndef MELTDOWN_BASIC_PRIMITIVES_H
#define MELTDOWN_BASIC_PRIMITIVES_H

#include <stdint.h>

int crosstalk_init(int argc, char** args);

void crosstalk_cleanup();

int flush_cache(void *mem);

int lfb_read(void *mem);

void lfb_vector_read(void* mem, uint8_t* buf);

int lfb_read_offset(void *mem, int offset);

void lfb_partial_vector_read(void* mem, uint8_t* buf, int start, int end);

int lfb_read_basic(void *mem, int offset);

int uses_taa();

#endif //MELTDOWN_BASIC_PRIMITIVES_H
