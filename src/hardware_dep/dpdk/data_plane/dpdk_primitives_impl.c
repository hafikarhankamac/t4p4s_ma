// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "util_debug.h"


void check_hdr_is_valid(packet_descriptor_t* pd, header_instance_t hdr, const char* operation_txt) {
    if (unlikely(pd->headers[hdr].pointer == 0)) {
        debug("Cannot %s header " T4LIT(%s,header) ", as it is " T4LIT(invalid,warning) "\n", operation_txt, hdr_infos[hdr].name);
    }
}

/*void GET_INT32_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld) {
    check_hdr_is_valid(pd, hdr, "get");
    GET_INT32_AUTO(handle(header_desc_ins(pd, hdr), fld));
}

void EXTRACT_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), dst);
}

void EXTRACT_INT32_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_INT32_AUTO(handle(header_desc_ins(pd, hdr), fld), dst);
}

void EXTRACT_INT32_BITS_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_INT32_BITS(handle(header_desc_ins(pd, hdr), fld), dst);
}
*/

void MODIFY_BYTEBUF_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* src, int srclen) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_BYTEBUF_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), src, srclen);
}

/*void MODIFY_INT32_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* src, int srclen) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_INT32_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), src, srclen);
}

void MODIFY_INT32_INT32_BITS_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, uint32_t value32) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_INT32_INT32_BITS(handle(header_desc_ins(pd, hdr), fld), value32);
}
*/

// TODO simplify all other interface macros, too
void MODIFY_INT32_INT32_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t h, field_instance_t f, uint32_t value32) {
    MODIFY_INT32_INT32_AUTO(handle(header_desc_ins(pd, h), f), value32);
}

void EXTRACT_BYTEBUF(bitfield_handle_t fd, uint8_t* dst) {
    if (fd.bitoffset == 0 && fd.bitwidth == fd.bytewidth * 8) {
        memcpy(dst, fd.byte_addr, fd.bytewidth);
    }
    else {
        uint8_t bits_in_last_byte = (fd.bitoffset + fd.bitwidth) % 8; // i. e. offset of next field
        uint8_t remaining_bits = (8 - bits_in_last_byte) % 8;
        uint8_t current_byte = 0;

        for (int i = fd.bytecount - 1; i > 0; i--) {
            current_byte = (*(fd.byte_addr + i)) >> remaining_bits;

            // We add parts of the previous byte only if the end is unaligned
            if (bits_in_last_byte)
                current_byte |= ((*(fd.byte_addr + i - 1)) << bits_in_last_byte);

            // If the current offset is higher than the next offset we need to mask
            if (i == 1 && bits_in_last_byte && bits_in_last_byte < fd.bitoffset) {
                current_byte &= ((1 << (8 - (fd.bitoffset - bits_in_last_byte))) - 1);
            }

            memcpy(dst - (fd.bytecount - fd.bytewidth) + i, &current_byte, 1);
        }

        if (!bits_in_last_byte) {
            current_byte = (*(fd.byte_addr) >> remaining_bits) & ((1 << fd.bitoffset) - 1);
            memcpy(dst, &current_byte, 1);
        }

        // If the current offset is lower than the next offset we need to put the rest in the first byte
        else if (bits_in_last_byte > fd.bitoffset) {
            current_byte = (*(fd.byte_addr) >> remaining_bits) & ((1 << (bits_in_last_byte - fd.bitoffset)) - 1);
            memcpy(dst, &current_byte, 1);
        }
    }
}


void MODIFY_BYTEBUF_BYTEBUF(bitfield_handle_t dst_fd, uint8_t* src, uint8_t srclen) {
    /*TODO: If the src contains a signed negative value, than the following memset is incorrect*/
    uint8_t byte_length = (srclen + 7) / 8;

    if (dst_fd.bitoffset == 0 && dst_fd.bytewidth * 8 == dst_fd.bitwidth) {
        memcpy(dst_fd.byte_addr, src, byte_length);
    }
    else {
        uint8_t bits_in_last_byte = (dst_fd.bitoffset + dst_fd.bitwidth) % 8; // i. e. offset of next field
        uint8_t remaining_bits = (8 - bits_in_last_byte) % 8;
        uint8_t current_byte = 0;

        // The last byte will likely overlap with the next field
        current_byte = src[byte_length - 1] << remaining_bits;
        current_byte |= (*(dst_fd.byte_addr + dst_fd.bytecount - 1)) & ((1 << remaining_bits) - 1);

        memcpy(dst_fd.byte_addr + dst_fd.bytecount - 1, &current_byte, 1);

        for (int i = byte_length - 1; i > 0; i--) {
            current_byte = src[i] >> bits_in_last_byte;
            current_byte |= src[i - 1] << remaining_bits;

            if (i == 1 && dst_fd.bitoffset < bits_in_last_byte) {
                current_byte &= ((1 << (8 - dst_fd.bitoffset)) - 1);
                current_byte |= (*dst_fd.byte_addr) & (((1 << dst_fd.bitoffset) - 1) << (8 - dst_fd.bitoffset));
            }

            memcpy(dst_fd.byte_addr - (1 - (dst_fd.bytecount - dst_fd.bytewidth)) + i, &current_byte, 1);
        }

        // If the bit offset is higher (within the byte) than the next one, src will be one byte shorter than
        // the bytes occupied by the field. We have to fill in the last field now.
        if (dst_fd.bitoffset && dst_fd.bitoffset >= bits_in_last_byte) {
            current_byte = (src[0] >> bits_in_last_byte) & ((1 << (8 - dst_fd.bitoffset)) - 1);
            current_byte |= (*(dst_fd.byte_addr)) & (((1 << dst_fd.bitoffset) - 1) << (8 - dst_fd.bitoffset) );
            memcpy(dst_fd.byte_addr, &current_byte, 1);
        }
    }
}


void set_field(fldT f[], bufT b[], uint32_t value32, int bit_width) {
#ifdef T4P4S_DEBUG
    // exactly one of `f` and `b` have to be non-zero
    assert((f == 0) != (b == 0));
#endif

    if (f != 0) {
        fldT fld = f[0];
        int byte_width = (bit_width+7)/8;

        debug("    " T4LIT(=,field) " Set " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%d) "b (" T4LIT(%d) "B) = " T4LIT(%d) " (0x" T4LIT(%0*x) ")\n",
              header_instance_names[fld.hdr],
              field_names[fld.fld],
              bit_width,
              byte_width,
              value32,
              2 * byte_width,
              value32);

        MODIFY_INT32_INT32_AUTO_PACKET(fld.pd, fld.hdr, fld.fld, value32);
    }

    // TODO implement this case, too
    if (b != 0)   rte_exit(2, "TODO unimplemented portion of set_field");
}
