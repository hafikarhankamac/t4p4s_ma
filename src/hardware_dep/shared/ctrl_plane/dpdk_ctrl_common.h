// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

typedef struct {
    uint8_t ip[4];
    uint16_t prefix_length;
    int i1;
    int i2;
    int i3;
} digest_ip_t;

typedef struct {
    uint8_t mac[6];
    uint16_t port;
} digest_macport_t;


typedef void (*digest_fun_t)(void* digest);

typedef struct {
    const char* name;
    digest_fun_t digest_fun;
} digest_dispatch_t;

// ------------------

void undigest_macport(digest_macport_t* dig, void* digest);
void undigest_ip(digest_ip_t* dig, void* digest);

// ------------------

int send_exact_entry(uint8_t port, uint8_t mac[6], const char* table_name, const char* header_name, const char* action_name, const char* par1, const char* par2);
int send_lpm_entry(uint8_t ip[4], uint16_t prefix_length, const char* table_name, const char* header_name, const char* action_name, int i1, int i2, int i3);
int send_ternary_ipv4_entry(uint8_t ip[4], uint8_t mask[4], uint8_t priority, const char* table_name, const char* header_name, const char* action_name);
int send_ternary_bits_entry(uint8_t num_of_bytes, uint8_t* bitmap, uint8_t* mask, uint8_t priority, const char* table_name, const char* header_name, const char* action_name);
int send_ternary_palmtrie_ipv4_entry(uint8_t ip[4], uint32_t mask, uint8_t priority, const char* table_name, const char* header_name, const char* action_name);
int send_ternary_palmtrie_bits_entry(uint8_t num_of_bytes, uint8_t* bitmap, uint8_t* mask, uint8_t priority, const char* table_name, const char* header_name, const char* action_name);
int send_ternary_abv_ipv4_entry(uint8_t ip[4], uint32_t mask, uint8_t priority, const char* table_name, const char* header_name, const char* action_name);
int send_ternary_egtpc_ipv4_entry(uint8_t ip[4], uint32_t mask, uint8_t priority, const char* table_name, const char* header_name, const char* action_name);

int fill_teid_rate_limiter_table(uint32_t teid, const char* table_name, const char* header_name, const char* mode);

// ------------------

void notify_controller_initialized();

void set_table_default_action(const char* table_nickname, const char* table_name, const char* default_action_name);
