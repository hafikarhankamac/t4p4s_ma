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

int process_ternary(const char* line) {
    char table_name[100];
    uint8_t ip[4];
    uint8_t mask[4];
    uint8_t priority;

    int matches = sscanf(line, "%*s %s %hhd.%hhd.%hhd.%hhd %02hhx%02hhx%02hhx%02hhx %d", table_name, &ip[0], &ip[1], &ip[2], &ip[3], &mask[0], &mask[1], &mask[2], &mask[3], &priority);
    //printf("IP: %hhd.%hhd.%hhd.%hhd Mask: %02hhx%02hhx%02hhx%02hhx Priority: %d\n", ip[0], ip[1], ip[2], ip[3], mask[0], mask[1], mask[2], mask[3], priority);
    if (10 != matches) return -1;

    send_ternary_entry(ip, mask, priority, table_name, "ipv4.srcAddr", "ingress.allow");
}

typedef int (*config_line_processor_t)(const char* line);

typedef struct config_processor_s {
    char name[64];
    config_line_processor_t process;
} config_processor_t;

config_processor_t code_processors[] = {
    { "SET-DEFAULT", process_set_default },
    { "TERNARY", process_ternary },
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