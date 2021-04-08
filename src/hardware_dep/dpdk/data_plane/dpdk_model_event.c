// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_event.h"
#include "util_packet.h"

void set_event_metadata(packet_descriptor_t* pd, event_e event, uint32_t args)
{
    int32_t res32; // needed for the macro
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EVENT_META_FLD, event);
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EVENT_ARG_META_FLD, args);
}


