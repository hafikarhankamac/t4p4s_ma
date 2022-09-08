// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include "dpdk_controller_dictionary.h"
#include "dpdk_ctrl_common.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_MACS 60000
#define MAX_IPS 60000

/*
 * Xorshift
 * Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
*/

uint32_t xor32_state;

static __inline__ uint32_t
xor32(uint32_t state)
{
	uint64_t x = state;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;

	return xor32_state = x;
}

uint64_t xor64_state;

static __inline__ uint64_t
xor64(uint64_t state)
{
	uint64_t x = state;

	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;

	return xor64_state = x;
}

static __inline__ uint32_t
xor128(void)
{
    static uint32_t x = 123456789;
    static uint32_t y = 362436069;
    static uint32_t z = 521288629;
    static uint32_t w = 88675123;
    uint32_t t;

    t = x ^ (x<<11);
    x = y;
    y = z;
    z = w;

    return w = (w ^ (w>>19)) ^ (t ^ (t >> 8));
}

controller c;

void dhf(void* b) {

    printf("Unknown digest received\n");
}

int process_set_default(const char* line) {
    char table_name[100];
    char default_action_name[100];

    int matches = sscanf(line, "%*s %s %s", table_name, default_action_name);
    if (2 != matches) return -1;

    set_table_default_action(table_name, table_name, default_action_name);

    return 0;
}

int process_ternary_ipv4(const char* line) {
    char table_name[100];
    uint8_t ip[4];
    //uint8_t mask[4];
    uint32_t mask;
    uint8_t priority;

    int matches = sscanf(line, "%*s %s %hhd.%hhd.%hhd.%hhd/%hhd %hhd", table_name, &ip[0], &ip[1], &ip[2], &ip[3], &mask, &priority);
    if (7 != matches) return -1;
 
    printf("Process PALMTRIE-IPv4 - IP: %hhd.%hhd.%hhd.%hhd Mask: %hhd Priority: %hhd\n", ip[0], ip[1], ip[2], ip[3], mask, priority);
 
    //send_ternary_palmtrie_ipv4_entry(ip, mask, priority, table_name, "ipv4.dstAddr", ".reflect");
    send_ternary_palmtrie_ipv4_entry(ip, mask, priority, table_name, "payload.lookup", ".reflect");

    return 0;
}

int process_ternary_bits(const char* line) {
    char table_name[100];
    uint8_t bitmap[100];
    uint8_t mask[100];
    uint8_t num_of_bytes;
    uint8_t priority;

    int matches = sscanf(line, "%*s %s %hhd %32s %32s %hhd", table_name, &num_of_bytes, &bitmap[0], &mask[0], &priority);
    if (5 != matches) return -1;
 
    printf("Process PALMTRIE-BITS - Num of Bytes: %hhd Bitmap: %32s Mask: %32s Priority: %hhd\n", num_of_bytes, bitmap, mask, priority);
 
    send_ternary_palmtrie_bits_entry(num_of_bytes, bitmap, mask, priority, table_name, "payload.lookup", ".reflect");

    return 0;
}

int process_random_bits(const char* line) {
    char table_name[100];
    int table_size;
    uint8_t num_of_bytes;

    int matches = sscanf(line, "%*s %s %hhd", table_name, &table_size, &num_of_bytes);
    if (3 != matches) return -1;

    printf("Process RANDOM-BITS - Table Size: %hhd Byte Size: %hhd\n", table_size, num_of_bytes);

    xor64_state = 88172645463325252LL;

    //for ( int i = 0; i < table_size; i++) {
        //send_ternary_palmtrie_random_bits_entry(ip, htonl((1ULL << (32 - (rand() % (32 - 1 + 1)) + 1)) - 1), table_name, "payload.lookup", ".reflect");
    //}

    return 0;
}

typedef int (*config_line_processor_t)(const char* line);

typedef struct config_processor_s {
    char name[64];
    config_line_processor_t process;
} config_processor_t;

config_processor_t code_processors[] = {
    { "SET-DEFAULT", process_set_default },
    { "TERNARY-IPv4", process_ternary_ipv4 },
    { "TERNARY-BITS", process_ternary_bits },
    { "RANDOM-BITS", process_random_bits },
    { {0}, 0 },
};

int process_config_file(FILE *f) {
    char line[100];
    char format_code[256];

    int line_index = 0;
    while (fgets(line, sizeof(line), f)) {
        line[strlen(line)-1] = '\0';
        ++line_index;

        // skipping empty lines and comments (lines that do not begin with a format code)
        // if (0 == strlen(line))   continue;
        // if (0 == sscanf(line, "%*[^a-zA-Z]"))   continue;

        int error_code = (1 == sscanf(line, "%s ", format_code));
        if (!error_code) {
            printf("Line %d: missing format code, skipping\n", line_index);
            continue;
        }

        for (int i = 0; code_processors[i].process != 0; ++i) {
            if (strcmp(code_processors[i].name, format_code) == 0) {
                error_code = code_processors[i].process(line);
                // "continue"s outer loop
                goto outer_loop_end;
            }
        }

        printf("Line %d: unknown format code %s\n", line_index, format_code);
        continue;

        outer_loop_end:
            if (error_code != 0) {
                printf("Line %d: could not process line (error code %d): %s\n", line_index, error_code, line);
            }
    }

    fclose(f);

    return 0;
}


#define MAX_CONFIG_FILES 64
static FILE* config_files[MAX_CONFIG_FILES] = { 0 };
static char config_file_names[100][MAX_CONFIG_FILES] = { "" };

void init() {
    for (int i = 1; i < MAX_CONFIG_FILES; ++i) {
        if (config_files[i] == 0)   break;
        printf("Processing config file %s\n", config_file_names[i]);
        process_config_file(config_files[i]);
    }

    printf("Done processing all config files\n");

    notify_controller_initialized();
}


int init_args(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        printf("Opening config file %s\n", argv[i]);
        config_files[i] = fopen(argv[i], "r");
        if (config_files[i] == 0) {
            printf("Error: cannot open config file %s\n", argv[i]);
            return -1;
        }
        strcpy(config_file_names[i], argv[i]);
        printf("Copied %s\n", config_file_names[i]);
    }


    printf("All config files opened\n");

    return 0;
}

int main(int argc, char* argv[])
{
    printf("Controller main started\n");

    int error_code = init_args(argc, argv);
    if (error_code < 0)    return error_code;

    printf("Create and configure controller...\n");
    c = create_controller_with_init(11111, 3, dhf, init);

    printf("Launching controller's main loop...\n");
    execute_controller(c);

    printf("Destroy controller\n");
    destroy_controller(c);

    return 0;
}
