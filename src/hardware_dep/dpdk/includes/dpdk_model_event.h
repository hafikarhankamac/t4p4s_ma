// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "backend.h"

#include "util_packet.h"
#include "common.h"

#define EVENT_META_FLD      FLD(all_metadatas,event)
#define EVENT_ARG_META_FLD  FLD(all_metadatas,event_arg)
#define EVENT_ARG_META2_FLD  FLD(all_metadatas,event_arg2)

#define NONE 0
#define TIMER 1

#define MAX_EVENT_BURST   32 

typedef uint8_t event_e;

typedef struct {
    event_e event : 8;
    uint64_t args : 56;
} event_s;

typedef union {
	event_s as_event;
	void* as_ptr;
} event_t;

void set_event_metadata(packet_descriptor_t* pd, event_s event);
void enque_event(struct rte_ring *event_queue, event_e event, uint64_t args);
void fill_event_queue(struct rte_ring *event_queue, event_e event, uint64_t args, uint16_t count);
