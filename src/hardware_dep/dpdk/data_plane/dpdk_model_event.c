// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_event.h"
#include "util_packet.h"


void set_event_metadata(packet_descriptor_t* pd, event_e event, uint64_t args)
{
    int32_t res32; // needed for the macro
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EVENT_META_FLD, event);
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EVENT_ARG_META_FLD, args);
}

void enque_event(struct rte_ring *event_queue, event_e event, uint64_t args)
{
    event_t e;
    e.as_event.event = event;
    e.as_event.args = args;
    rte_ring_enqueue(event_queue, e.as_ptr);
}

void fill_event_queue(struct rte_ring *event_queue, event_e event, uint64_t args, const uint16_t count)
{
    event_t events [count];
    for (uint16_t i = 0; i < count; i++) {
    	events[i].as_event.event = event;
	events[i].as_event.args = args;
    }

    rte_ring_mp_enqueue_bulk(event_queue, &events, count, NULL);
}
