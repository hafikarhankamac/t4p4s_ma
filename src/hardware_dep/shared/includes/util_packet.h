// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#pragma once

#define SHORT_STDPARAMS packet_descriptor_t* pd, lookup_table_t** tables
#define SHORT_STDPARAMS_IN pd, tables
#define STDPARAMS SHORT_STDPARAMS, parser_state_t* pstate
#define STDPARAMS_IN SHORT_STDPARAMS_IN, pstate

#define LCPARAMS struct lcore_data* lcdata, packet_descriptor_t* pd
#define LCPARAMS_IN lcdata, pd

typedef struct bitfield_handle_s {
    uint8_t* byte_addr;
    int      meta; // endianness / is_host_byte_order
    int      bitwidth;
    int      bytewidth;
    int      bitcount;
    int      bytecount;
    int      bitoffset;
    int      byteoffset;
    uint32_t mask;
    int      fixed_width;
} bitfield_handle_t;


typedef struct packet_descriptor_s packet_descriptor_t;
typedef struct header_descriptor_s header_descriptor_t;
typedef struct header_reference_s  header_reference_t;
typedef struct field_reference_s   field_reference_t;


typedef struct uint8_buffer_s {
       int      buffer_size;
       uint8_t* buffer;
} uint8_buffer_t;
