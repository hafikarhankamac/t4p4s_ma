// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "backend.h"

#include "util_packet.h"
#include "common.h"

#define EVENT_META_FLD      FLD(all_metadatas,event)
#define EVENT_ARG_META_FLD  FLD(all_metadatas,event_arg)

enum EVENTS {
    NONE = 0,
    EVENT1 = 1,
    EVENT2 = 2
};

typedef enum EVENTS event_e;

struct event_s {
    event_e event;
    uint32_t args;
};

typedef struct event_s event_t;

void set_event_metadata(packet_descriptor_t* pd, event_e event, uint32_t args);
