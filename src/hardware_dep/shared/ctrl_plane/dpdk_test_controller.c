// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_MACS 60000

controller c;

extern void notify_controller_initialized();

void fill_smac_table(uint32_t count, uint8_t mac[6])
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap;
    // struct p4_field_match_header* fmh;
    struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "table0_0");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "ethernet.srcAddr");
    memcpy(exact->bitmap, mac, 6);
    exact->length = 6*8+0;



    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "forward");

    ap = add_p4_action_parameter(h, a, 2048);
    strcpy(ap->name, "count");
    memcpy(ap->bitmap, &count, 4);
    ap->length = 4*8;


    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);

    send_p4_msg(c, buffer, 2048);
}



void dhf(void* b) {
    printf("Unknown digest received\n");
}

void set_default_action_smac()
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_set_default_action* sda;
    struct p4_action* a;

    printf("Generate set_default_action message for table smac\n");

    h = create_p4_header(buffer, 0, sizeof(buffer));

    sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
    strcpy(sda->table_name, "table0_0");

    a = &(sda->action);
    strcpy(a->description.name, "_drop");

    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, sizeof(buffer));
}


uint8_t macs[MAX_MACS][6];
uint32_t countmap[MAX_MACS];
int mac_count = -1;

int read_macs_and_ports_from_file(char *filename) {
    FILE *f;
    char line[100];
    int values[6];
    uint32_t count;
    int i;

    f = fopen(filename, "r");
    if (f == NULL) return -1;

    while (fgets(line, sizeof(line), f)) {
        line[strlen(line)-1] = '\0';

        if (7 == sscanf(line, "%x:%x:%x:%x:%x:%x %d",
                        &values[0], &values[1], &values[2],
                        &values[3], &values[4], &values[5], &count) )
        {
            if (mac_count==MAX_MACS-1)
            {
                printf("Too many entries...\n");
                break;
            }

            ++mac_count;
            for( i = 0; i < 6; ++i ) {
              macs[mac_count][i] = (uint8_t) values[i];
            }
            countmap[mac_count] = (uint32_t) count;

        } else {
            printf("Wrong format error in line %d : %s\n", mac_count+2, line);
            fclose(f);
            return -1;
        }

    }

    fclose(f);
    return 0;
}

void init() {
    int i;
    printf("Set default actions.\n");
    set_default_action_smac();

    for (i=0;i<=mac_count;++i)
    {
        printf("Filling tables smac COUNT: %d MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", countmap[i], macs[i][0],macs[i][1],macs[i][2],macs[i][3],macs[i][4],macs[i][5]);
        fill_smac_table(countmap[i], macs[i]);
    }

    notify_controller_initialized();
}


int main(int argc, char* argv[])
{
    if (argc>1) {
        if (argc!=2) {
            printf("Too many arguments...\nUsage: %s <filename(optional)>\n", argv[0]);
            return -1;
        }
        printf("Command line argument is present...\nLoading configuration data...\n");
        if (read_macs_and_ports_from_file(argv[1])<0) {
            printf("File cannnot be opened...\n");
            return -1;
        }
    }

    printf("Create and configure controller...\n");
    c = create_controller_with_init(11111, 3, dhf, init);

    printf("Launching controller's main loop...\n");
    execute_controller(c);

    printf("Destroy controller\n");
    destroy_controller(c);

    return 0;
}

