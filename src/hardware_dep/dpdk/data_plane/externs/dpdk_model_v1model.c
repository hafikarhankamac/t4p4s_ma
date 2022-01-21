// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include "sheep_precise_timer.h"

#ifdef TIMER_MODULE
	#include "timer_extern.h"
#endif


#ifdef EVENT_MODULE
    extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];
#endif

#include <rte_ip.h>

void transfer_to_egress(packet_descriptor_t* pd)
{
}

int extract_egress_port(packet_descriptor_t* pd) {
    return GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD);
}

int extract_ingress_port(packet_descriptor_t* pd) {
    return GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), INGRESS_META_FLD);
}

void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid)
{
    int res32; // needed for the macro
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), INGRESS_META_FLD, portid);
}

void mark_to_drop(SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(mark_to_drop,extern) "\n");

    uint32_t res32;
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_DROP_VALUE)

    debug("       : " T4LIT(all_metadatas,header) "." T4LIT(EGRESS_META_FLD,field) " = " T4LIT(EGRESS_DROP_VALUE,bytes) "\n");
}

void sheep(uint32_t *duration, SHORT_STDPARAMS) {
    debug(" :::: Called extern " T4LIT(sheep,extern) " waiting " T4LIT(%d) " cycles\n", duration);
    wait_cycles(*duration);
}

#ifdef EVENT_MODULE
void raise_event(uint8_t *event_id, uint32_t *args, SHORT_STDPARAMS) {
    debug(" :::: Called extern " T4LIT(raise_event,extern) " raising event " T4LIT(%d) " with args " T4LIT(%d) "\n", *event_id, *args);
    enque_event(lcore_conf[rte_lcore_id()].state.event_queue, *event_id, *args);
}
#endif

#ifdef TIMER_MODULE
void timer_single(uint32_t duration, uint32_t id, SHORT_STDPARAMS) {
    debug(" :::: Called extern " T4LIT(timer_single,extern) " waiting " T4LIT(%d) " ms with ID " T4LIT(%d) " \n", duration, id);
    single_timer(&duration, &id);
}

void timer_periodic(uint32_t duration, uint32_t id, SHORT_STDPARAMS) {
    debug(" :::: Called extern " T4LIT(timer_periodic,extern) " waiting " T4LIT(%d) " ms with ID " T4LIT(%d) " \n", duration, id);
    periodic_timer(&duration, &id);
}

void timer_multiple(uint32_t duration, uint32_t id, uint32_t count, SHORT_STDPARAMS) {
    debug(" :::: Called extern " T4LIT(timer_multiple,extern) " waiting " T4LIT(%d) " ms with ID " T4LIT(%d) " \n", duration, id);
    multiple_timer(&duration, &id, &count);
}

void timer_burst(uint32_t id, SHORT_STDPARAMS) {
    debug(" :::: Called extern " T4LIT(timer_burst,extern) "with ID " T4LIT(%d) " \n", id);
    burst_timer(&id);
}
#endif
