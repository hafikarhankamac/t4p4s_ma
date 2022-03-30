// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_event.h"
#include "util_packet.h"


void set_event_metadata(packet_descriptor_t* pd, event_s event)
{
    int32_t res32; // needed for the macro
    event_e event_event =  event.event;
    uint32_t args_shift =  (uint32_t) event.args;
    uint32_t args2 = (uint32_t) (event.args >> 32);
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EVENT_META_FLD, event_event);
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EVENT_ARG_META_FLD, args_shift);
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EVENT_ARG_META2_FLD, args2);
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
