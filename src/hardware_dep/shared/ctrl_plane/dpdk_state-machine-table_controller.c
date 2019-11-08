// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_MACS 60000

controller c;

void notify_controller_initialized()
{
        char buffer[sizeof(struct p4_header)];
        struct p4_header* h;

        h = create_p4_header(buffer, 0, sizeof(struct p4_header));
        h->type = P4T_CTRL_INITIALIZED;

        netconv_p4_header(h);

        send_p4_msg(c, buffer, sizeof(struct p4_header));
}

void fill_dmac_table(uint8_t port, uint8_t mac[6])
{
        char buffer[2048];
        struct p4_header* h;
        struct p4_add_table_entry* te;
        struct p4_action* a;
        struct p4_action_parameter* ap;
        struct p4_field_match_exact* exact;

        h = create_p4_header(buffer, 0, 2048);
        te = create_p4_add_table_entry(buffer,0,2048);
        strcpy(te->table_name, "dmac_0");

        exact = add_p4_field_match_exact(te, 2048);
        strcpy(exact->header.name, "ethernet.dstAddr");
        memcpy(exact->bitmap, mac, 6);
        exact->length = 6*8+0;

        a = add_p4_action(h, 2048);
        strcpy(a->description.name, "forward");

        ap = add_p4_action_parameter(h, a, 2048);
        strcpy(ap->name, "port");
        memcpy(ap->bitmap, &port, 1);
        ap->length = 1*8+0;

        netconv_p4_header(h);
        netconv_p4_add_table_entry(te);
        netconv_p4_field_match_exact(exact);
        netconv_p4_action(a);
        netconv_p4_action_parameter(ap);

        send_p4_msg(c, buffer, 2048);
}

void fill_state_table(uint16_t state, char *function)
{
        char buffer[2048];
        struct p4_header* h;
        struct p4_add_table_entry* te;
        struct p4_action* a;
        struct p4_field_match_exact* exact;

        h = create_p4_header(buffer, 0, 2048);
        te = create_p4_add_table_entry(buffer,0,2048);
        strcpy(te->table_name, "switch_state_0");

        exact = add_p4_field_match_exact(te, 2048);
        strcpy(exact->header.name, "state_metadata.current_state");
        memcpy(exact->bitmap, &state, 2);
        exact->length = 2*8+0;

        a = add_p4_action(h, 2048);
        strcpy(a->description.name, function);

        netconv_p4_header(h);
        netconv_p4_add_table_entry(te);
        netconv_p4_field_match_exact(exact);
        netconv_p4_action(a);

        send_p4_msg(c, buffer, 2048);
}

void fill_map_flow_to_state_table(uint8_t flow_id[4], uint8_t state[2])
{
        char buffer[2048];
        struct p4_header* h;
        struct p4_add_table_entry* te;
        struct p4_action* a;
        struct p4_action_parameter* ap;
        struct p4_field_match_exact* exact;

        h = create_p4_header(buffer, 0, 2048);
        te = create_p4_add_table_entry(buffer,0,2048);
        strcpy(te->table_name, "map_flow_to_state_0");

        exact = add_p4_field_match_exact(te, 2048);
        strcpy(exact->header.name, "state_metadata.flow_id");
        memcpy(exact->bitmap, flow_id, 4);
        exact->length = 4*8+0;

        a = add_p4_action(h, 2048);
        strcpy(a->description.name, "existing_flow");

        ap = add_p4_action_parameter(h, a, 2048);
        strcpy(ap->name, "state");
        memcpy(ap->bitmap, state, 2);
        ap->length = 2*8+0;

        netconv_p4_header(h);
        netconv_p4_add_table_entry(te);
        netconv_p4_field_match_exact(exact);
        netconv_p4_action(a);
        netconv_p4_action_parameter(ap);

        send_p4_msg(c, buffer, 2048);
}

void modify_map_flow_to_state_table(uint8_t flow_id[4], uint8_t state[2])
{
    // modify NYI by t4p4s, simply overwrite
    fill_map_flow_to_state_table(flow_id, state);
}

void state_update_digest(void* b) {
    uint16_t state[1];
    uint32_t flow_id[1];
    uint8_t new_flow[1];
    uint16_t offset=0;
    offset = sizeof(struct p4_digest);
    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(state, df->value, 2);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(flow_id, df->value, 4);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(new_flow, df->value, 1);

    // new_flow is a 1 bit value
    new_flow[0] = new_flow[0] >> 7;

    printf("Ctrl: state_update_digest STATE: %d FLOW_ID: %d (%d.%d.%d.%d) NEW: %d\n", *state, flow_id[0], ((uint8_t *)flow_id)[0], ((uint8_t *)flow_id)[1], ((uint8_t *)flow_id)[2], ((uint8_t *)flow_id)[3], new_flow[0]);

    if (*new_flow) {
        fill_map_flow_to_state_table((uint8_t *)flow_id, (uint8_t *)state);
    } else {
        modify_map_flow_to_state_table((uint8_t *)flow_id, (uint8_t *)state);
    }
}

void dhf(void* b) {
    struct p4_header* h = netconv_p4_header(unpack_p4_header(b, 0));
    if (h->type != P4T_DIGEST) {
        printf("Method is not implemented\n");
        return;
    }

    struct p4_digest* d = unpack_p4_digest(b,0);
    if (strcmp(d->field_list_name, "state_update_digest")==0) {
        state_update_digest(b);
    } else {
        printf("Unknown digest received: X%sX\n", d->field_list_name);
    }
}

void set_default_action_dmac()
{
        char buffer[2048];
        struct p4_header* h;
        struct p4_set_default_action* sda;
        struct p4_action* a;
        struct p4_action_parameter* ap;
        uint8_t port = 0;

        printf("Generate set_default_action message for table dmac\n");

        h = create_p4_header(buffer, 0, sizeof(buffer));

        sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
        strcpy(sda->table_name, "dmac_0");

        a = &(sda->action);
        strcpy(a->description.name, "forward");

        ap = add_p4_action_parameter(h, a, 2048);
        strcpy(ap->name, "port");
        memcpy(ap->bitmap, &port, 1);
        ap->length = 1*8+0;

        netconv_p4_header(h);
        netconv_p4_set_default_action(sda);
        netconv_p4_action(a);
        netconv_p4_action_parameter(ap);

        send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_map_flow_to_state()
{
        char buffer[2048];
        struct p4_header* h;
        struct p4_set_default_action* sda;
        struct p4_action* a;

        printf("Generate set_default_action message for table map_flow_to_state\n");

        h = create_p4_header(buffer, 0, sizeof(buffer));

        sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
        strcpy(sda->table_name, "map_flow_to_state_0");

        a = &(sda->action);
        strcpy(a->description.name, "new_flow");

        netconv_p4_header(h);
        netconv_p4_set_default_action(sda);
        netconv_p4_action(a);

        send_p4_msg(c, buffer, sizeof(buffer));
}

void init() {
        printf("Set default actions.\n");
        set_default_action_dmac();
        set_default_action_map_flow_to_state();

        printf("Fill tables.\n");
        fill_state_table(0, "new_state");
        fill_state_table(1, "state1");
        fill_state_table(2, "state2");
        fill_state_table(3, "state3");
        fill_state_table(4, "state4");
        fill_state_table(5, "state5");

        notify_controller_initialized();
}


int main(int argc, char* argv[])
{
        if (argc>1) {
            printf("Too many arguments...\nUsage: %s\n", argv[0]);
            return -1;
        }

        printf("Create and configure controller...\n");
        c = create_controller_with_init(11111, 3, dhf, init);

        printf("Launching controller's main loop...\n");
        execute_controller(c);

        printf("Destroy controller\n");
        destroy_controller(c);

        return 0;
}

