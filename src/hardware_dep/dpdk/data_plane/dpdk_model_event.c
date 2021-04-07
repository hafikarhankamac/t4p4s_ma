// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_event.h"
#include "util_packet.h

void set_event_metadata(packet_descriptor_t* pd, uint8_t event, uint64_t args)
{
    uint8_t res8; // needed for the macro
    uint64_t res64; // needed for the macro
    MODIFY_INT8_INT8_BITS_PACKET(pd, HDR(all_metadatas), EVENT_META_FLD, event);
    MODIFY_INT64_INT64_BITS_PACKET(pd, HDR(all_metadatas), EVENT_ARG_META_FLD, args);
}


