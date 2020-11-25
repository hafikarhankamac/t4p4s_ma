// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "sheep_precise_timer.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_ip.h>

void transfer_to_egress(packet_descriptor_t* pd)
{
    // int res32; // needed for the macro
    // uint32_t val = GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD);
    // MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, val);
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

void verify_checksum(bool cond, struct uint8_buffer_s data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(verify_checksum,extern) "\n");
    uint32_t res32, current_cksum = 0, calculated_cksum = 0;
    if (cond) {
        if (algorithm == enum_HashAlgorithm_csum16) {
            calculated_cksum = rte_raw_cksum(data.buffer, data.buffer_size);
            calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
            EXTRACT_INT32_BITS(cksum_field_handle, current_cksum)
        }

#ifdef T4P4S_DEBUG
        if (current_cksum == calculated_cksum) {
            debug("      : Packet checksum is " T4LIT(ok,success) ": " T4LIT(%04x,bytes) "\n", current_cksum);
        } else {
            debug("    " T4LIT(!!,error) " Packet checksum is " T4LIT(wrong,error) ": " T4LIT(%04x,bytes) ", calculated checksum is " T4LIT(%04x,bytes) "\n", current_cksum, calculated_cksum);
        }
#endif

        if (unlikely(calculated_cksum != current_cksum)) {
            MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,checksum_error), 1)
        }
    }
}

void update_checksum(bool cond, struct uint8_buffer_s data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(update_checksum,extern) "\n");

    uint32_t res32, calculated_cksum = 0;
    if(cond) {
        if (algorithm == enum_HashAlgorithm_csum16) {
            calculated_cksum = rte_raw_cksum(data.buffer, data.buffer_size);
            calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
        }

        debug("       : Packet checksum " T4LIT(updated,status) " to " T4LIT(%04x,bytes) "\n", calculated_cksum);

        // TODO temporarily disabled: this line modifies a lookup table's pointer instead of a checksum field
        MODIFY_INT32_INT32_BITS(cksum_field_handle, calculated_cksum)
    }
}

void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(verify_checksum_offload,extern) "\n");

    if ((pd->wrapper->ol_flags & PKT_RX_IP_CKSUM_BAD) != 0) {
        uint32_t res32;
        MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,checksum_error), 1)

        debug("       : Verifying packet checksum: " T4LIT(%04x,bytes) "\n", res32);
    }
}

void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(update_checksum_offload,extern) "\n");

    pd->wrapper->l2_len = len_l2;
    pd->wrapper->l3_len = len_l3;
    pd->wrapper->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM;
    uint32_t res32;
    MODIFY_INT32_INT32_BITS(cksum_field_handle, 0)

    debug("       : Updating packet checksum (offload)\n");
    // TODO implement offload
}

void mark_to_drop(SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(mark_to_drop,extern) "\n");

    uint32_t res32;
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_DROP_VALUE)

    debug("       : " T4LIT(all_metadatas,header) "." T4LIT(EGRESS_META_FLD,field) " = " T4LIT(EGRESS_DROP_VALUE,bytes) "\n");
}


void sheep(uint32_t duration, SHORT_STDPARAMS) {
    debug(" :::: Called extern " T4LIT(sheep,extern) " waiting " T4LIT(%d) " cycles\n", duration);
    wait_cycles(duration);
}

//void encrypt_bytes(enum enum_EncryptionAlgorithm algorithm, enum enum_EncryptionMode mode, uint32_t iv, uint32_t key, uint16_t start_byte, uint16_t length, SHORT_STDPARAMS) {
//    debug(" :::: Called extern " T4LIT(encrypt_bytes,extern) " (" T4LIT(%d) "-" T4LIT(%d) ") [" T4LIT(IV, field) " " T4LIT(%x) " " T4LIT(key, field) " " T4LIT(%x) "] starting at byte " T4LIT(%d) " (" T4LIT(%d) " bytes)\n", algorithm, mode, iv, key, start_byte, length);
//
//    for (uint16_t i = 0 ; i < length; i++) {
//        uint16_t idx = start_byte + i;
//        pd->data[idx] = pd->data[idx] + key + iv;
//    }
//    sheep((uint32_t) length);
//}

void verify(bool check, error_error_t toSignal, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify,extern) "\n");
}

void verify_checksum_with_payload(bool condition, struct uint8_buffer_s data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify_checksum_with_payload,extern) "\n");
}

void update_checksum_with_payload(bool condition, struct uint8_buffer_s data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(update_checksum_with_payload,extern) "\n");
}


extern void do_counter_count(counter_t* counter, int index, uint32_t value);

void extern_counter_count(uint32_t counter_array_size, enum_CounterType_t ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS) {
    do_counter_count(counter, index, ct == enum_CounterType_packets ? 1 : packet_length(pd));
}

void extern_meter_execute_meter(uint32_t index, enum_MeterType_t b, uint32_t c, uint8_t d, meter_t e, SHORT_STDPARAMS) {
    debug("    : Executing extern_meter_execute_meter#" T4LIT(%d) "\n", index);
}

void extern_register_read(uint32_t index, uint32_t a, uint32_t b, register_t c, SHORT_STDPARAMS) {
    debug("    : Executing extern_register_read#" T4LIT(%d) "\n", index);
}

void extern_register_write(uint32_t index, uint32_t a, uint32_t b, register_t* c, SHORT_STDPARAMS) {
    debug("    : Executing extern_register_write#" T4LIT(%d) "\n", index);
}
