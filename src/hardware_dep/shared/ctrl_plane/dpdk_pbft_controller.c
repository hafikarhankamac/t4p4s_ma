// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_ENTRIES 1048576

#define TYPE uint32_t
#define SIZE  1

#define K 128
#define N 3

controller c;

extern void notify_controller_initialized();
void fill_table(TYPE count[], TYPE key, const char* table_name, const char* header_name, const char* action_name)
{
    char buffer[4096];
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap[SIZE];
    // struct p4_field_match_header* fmh;
    struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 4096);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, (table_name));

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, (header_name));
    memcpy(exact->bitmap, &key, sizeof(TYPE));
    exact->length = sizeof(TYPE)*8+0;



    a = add_p4_action(h, 2048);
    strcpy(a->description.name, (action_name));

    for (uint32_t i = 0; i < SIZE; i++) {
        ap[i] = add_p4_action_parameter(h, a, 2048);
        char name[11];
        sprintf(name, "count%05d", i);
        strcpy(ap[i]->name, (name));
        memcpy(ap[i]->bitmap, &count[i], (sizeof(TYPE)));
        ap[i]->length = (sizeof(TYPE)) * 8;
    }

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    
    for (uint32_t i = 0; i < SIZE; i++) {
	    netconv_p4_action_parameter(ap[i]);
    }

    send_p4_msg(c, buffer, 2048);
}

uint8_t SIZES_PARAMS[] = { 4, 4, 2, 2, 1, 1, 4 };
uint8_t SUM_SIZES = 4 + 4 + 2 + 2 + 1 + 1 + 4;
char* PARAMS_NAMES[] = { "view", "sn", "count_prepare", "count_commit", "bitmap_prepare", "bitmap_commit", "dig"};
uint8_t NUM_PARAMS = 7;

void fill_commit_deliver_table(const char* table_name, const char* action_name, const int lv, const int sn)
{
    char buffer[4096];
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap[NUM_PARAMS];
    // struct p4_field_match_header* fmh;
    struct p4_field_match_exact* exact;
    struct p4_field_match_exact* exact2;

    h = create_p4_header(buffer, 0, 4096);
    te = create_p4_add_table_entry(buffer,0,4096);
    strcpy(te->table_name, (table_name));

    exact = add_p4_field_match_exact(te, 4096);
    strcpy(exact->header.name, "meta.relative_sn");
    memcpy(exact->bitmap, &lv, 4);
    exact->length = 8*4;

    exact2 = add_p4_field_match_exact(te, 4096);
    strcpy(exact2->header.name, "meta.relative_lv");
    memcpy(exact2->bitmap, &sn, 4);
    exact2->length = 8*4;

    uint64_t ZERO = 0;

    a = add_p4_action(h, 4096);
    strcpy(a->description.name, (action_name));

    for (uint32_t i = 0; i < NUM_PARAMS; i++) {
        ap[i] = add_p4_action_parameter(h, a, 4096);
        strcpy(ap[i]->name, PARAMS_NAMES[i]);
        memcpy(ap[i]->bitmap, &ZERO, SIZES_PARAMS[i]);
        ap[i]->length = SIZES_PARAMS[i] * 8;
    }

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_field_match_exact(exact2);
    netconv_p4_action(a);

    for (uint32_t i = 0; i < NUM_PARAMS; i++) {
	    netconv_p4_action_parameter(ap[i]);
    }

    send_p4_msg(c, buffer, 4096);
}


void change_entry(TYPE count[], TYPE key) {
    char buffer[2048];
    struct p4_header* h;
    struct p4_change_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap[SIZE];
    struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_change_table_entry(buffer,0,2048);
    strcpy(te->table_name, ("ingress.table0_0"));

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, ("custom.payload1"));
    memcpy(exact->bitmap, &key, sizeof(TYPE));
    exact->length = sizeof(TYPE)*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, ".forward");

    for (uint32_t i = 0; i < SIZE; i++) {
        ap[i] = add_p4_action_parameter(h, a, 2048);
        char name[11];
        sprintf(name, "count%05d", i);
        strcpy(ap[i]->name, (name));
        memcpy(ap[i]->bitmap, &count[i], (sizeof(TYPE)));
        ap[i]->length = (sizeof(TYPE)) * 8;
    }


    netconv_p4_header(h);
    netconv_p4_change_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);

    for (uint32_t i = 0; i < SIZE; i++) {
        netconv_p4_action_parameter(ap[i]);
    }

    send_p4_msg(c, buffer, 2048);
}


void dhf(void* b) {
    struct p4_header* h = netconv_p4_header(unpack_p4_header(b, 0));
    if (h->type != P4T_DIGEST) {
        printf("Method is not implemented\n");
        return;
    }

        printf("Unknown digest received: \n");
}

void set_default_action(char *table_name, char *action_name)
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_set_default_action* sda;
    struct p4_action* a;

    printf("Generate set_default_action message for table forward_tab\n");

    h = create_p4_header(buffer, 0, sizeof(buffer));

    sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
    strcpy(sda->table_name, table_name);

    a = &(sda->action);
    strcpy(a->description.name, action_name);

    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, sizeof(buffer));
}


TYPE entry[MAX_ENTRIES];
TYPE countmap[MAX_ENTRIES][SIZE];
int entry_count = -1;

int read_entries_from_file(char *filename) {
    FILE *f;
    char line[200];
    TYPE key;
    int i;
    char* ptr;

    f = fopen(filename, "r");
    if (f == NULL) return -1;

    while (fgets(line, sizeof(line), f)) {
	    line[strlen(line)-1] = '\0';
	    if (entry_count==MAX_ENTRIES-1)
	    {
		    printf("Too many entries...\n");
		    break;
	    }

	    ptr = strtok(line, " ");
	    TYPE c;
	    i = 0;
	    if (ptr != NULL) {
		    sscanf(ptr, "%d", &c);
		    ptr = strtok(NULL, " ");
		    entry[entry_count] = (TYPE) c;
	    }
	    while(ptr != NULL) {
		    sscanf(ptr, "%d", &c);
		    ptr = strtok(NULL, " ");
		    if (i >= SIZE) {
		        printf("Too many entries...\n");
		        break;
		    }
		    countmap[entry_count][i++] = (TYPE) c;
	    }

	    entry_count++;
    }

    fclose(f);
    return 0;
}

void init() {
    int i;
    sleep(20);
    printf("Set default actions.\n");
    set_default_action("ingress.forward_tab_0", "._drop");
    set_default_action("ingress.commit_deliver_tab_0", ".commit_deliver");
    set_default_action("ingress.preprepare_tab_0", ".preprepare");
    set_default_action("ingress.process_tab_0", ".debug_print");
    set_default_action("ingress.checkpoint_tab_0", ".handle_checkpoint");

    //forward_tab

    for (int lv = 0; lv < K; lv++) {
        for (int sn = 0; sn < N; sn++) {
        printf("Filling commit_deliver_tab with sn %i lv %i -> commit_deliver 0 0 0 0 0 0 0", sn, lv);
            // fill_commit_deliver_table("ingress.commit_deliver_tab_0", ".commit_deliver", lv, sn);
        }
    }

   /* for (i=0;i<=entry_count;++i)
    {
        printf("Filling tables key: %08x\n", entry[i]);
        fill_table(countmap[i], entry[i],"ingress.table0_0", "custom.payload1", ".forward");
	if (i % 5000 == 0) {
	    sleep(2);
	}
    }
*/
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
        /*if (read_entries_from_file(argv[1])<0) {
            printf("File cannnot be opened...\n");
            return -1;
        }*/
    }

    printf("Create and configure controller...\n");
    c = create_controller_with_init(11111, 3, dhf, init);

    printf("Launching controller's main loop...\n");
    execute_controller(c);

    printf("Destroy controller\n");
    destroy_controller(c);

    return 0;
}
