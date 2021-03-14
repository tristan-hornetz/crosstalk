#ifndef MELTDOWN_BASIC_PRIMITIVES_H
#define MELTDOWN_BASIC_PRIMITIVES_H

#include <stdint.h>

int fallout_compatible();

int crosstalk_init(int argc, char** args);

void crosstalk_cleanup();

int flush_cache(void *mem);

int lfb_read(void *mem);

void lfb_vector_read(void* mem, uint8_t* buf);

#endif //MELTDOWN_BASIC_PRIMITIVES_H
