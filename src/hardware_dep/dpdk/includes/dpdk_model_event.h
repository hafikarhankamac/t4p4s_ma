// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "util_packet.h"
#include "common.h"

#define EVENT_META_FLD      FLD(all_metadatas,event)
#define EVENT_ARG_META_FLD  FLD(all_metadatas,event_arg)

void set_event_metadata(packet_descriptor_t* pd, uint8_t event, uint64_t args);

struct event_s {
    enum EVENTS event;
    uint64_t args;
} event_t;

typedef event_s event;

enum EVENTS {
    NONE = 0,
    EVENT1 = 1,
    EVENT2 = 2
};

typedef enum EVENTS event_e;