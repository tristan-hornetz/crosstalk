#ifndef RIDL_UTILS_H
#define RIDL_UTILS_H

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

#define TIMES_TEN(X) X X X X X X X X X X
int _page_size = 0x1000;
int writer_cpu = 3, reader_cpu = 7, offcore_cpu = 1, pid = 0, pid2 = 0;

uint32_t vector_hits[64][256];
uint8_t vector[64];


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

#endif //RIDL_UTILS_H
