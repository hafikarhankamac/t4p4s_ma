// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "util_debug.h"


void check_hdr_is_valid(packet_descriptor_t* pd, header_instance_t hdr, const char* operation_txt) {
    if (unlikely(pd->headers[hdr].pointer == 0)) {
        debug("Cannot %s header " T4LIT(%s,header) ", as it is " T4LIT(invalid,warning) "\n", operation_txt, hdr_infos[hdr].name);
    }
}

/*void GET_INT64_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld) {
    check_hdr_is_valid(pd, hdr, "get");
    GET_INT64_AUTO(handle(header_desc_ins(pd, hdr), fld));
}

void EXTRACT_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), dst);
}

void EXTRACT_INT64_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_INT64_AUTO(handle(header_desc_ins(pd, hdr), fld), dst);
}

void EXTRACT_INT64_BITS_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_INT64_BITS(handle(header_desc_ins(pd, hdr), fld), dst);
}
*/

void MODIFY_BYTEBUF_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* src, int srclen) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_BYTEBUF_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), src, srclen);
}

/*void MODIFY_INT64_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* src, int srclen) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_INT64_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), src, srclen);
}

void MODIFY_INT64_INT64_BITS_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, uint32_t value32) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_INT64_INT64_BITS(handle(header_desc_ins(pd, hdr), fld), value32);
}
*/

// TODO simplify all other interface macros, too
void MODIFY_INT64_INT64_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t h, field_instance_t f, uint64_t value64) {

    // header_desc_ins(pd, h) fetches the header from the packet
    // handle(header, f) collects all informations about the field

    MODIFY_INT64_INT64_AUTO(handle(header_desc_ins(pd, h), f), value64);
}

void MODIFY_INT64_INT64_AUTO(bitfield_handle_t dst_fd, uint64_t value64) {
    if (dst_fd.meta)
        MODIFY_INT64_INT64_BITS(dst_fd, value64)
    else
        MODIFY_INT64_INT64_HTON(dst_fd, value64)
}

void MODIFY_INT64_INT64_HTON(bitfield_handle_t dst_fd, uint64_t value64) {
    uint64_t res64 = (FLD_BYTES(dst_fd) & ~FLD_MASK(dst_fd));
    
    if (dst_fd.bytecount == 1)
        res64 |= (value64 << (8 - dst_fd.bitcount)) & FLD_MASK(dst_fd);
    else if (dst_fd.bytecount == 2)
        res64 |= rte_cpu_to_be_16(value64 << (16 - dst_fd.bitcount)) & FLD_MASK(dst_fd);
    else if (dst_fd.bytecount <= 4)
        res64 |= rte_cpu_to_be_32(value64 << (32 - dst_fd.bitcount)) & FLD_MASK(dst_fd);
    else
        res64 |= rte_cpu_to_be_64(value64 << (64 - dst_fd.bitcount)) & FLD_MASK(dst_fd);

    memcpy(dst_fd.byte_addr, &res64, dst_fd.bytecount);
}

void MODIFY_INT64_INT64_BITS(bitfield_handle_t dst_fd, uint64_t value64) {
    uint64_t res64 = (FLD_BYTES(dst_fd) & ~FLD_MASK(dst_fd));
    if (dst_fd.bytecount == 1) {
        res64 |= (value64 << (8 - dst_fd.bitcount) & FLD_MASK(dst_fd));
    } else if (dst_fd.bytecount == 2) {
        res64 |= MASK_AT(value64, MASK_LOW(dst_fd), 0);
        res64 |= MASK_AT(value64, MASK_TOP(dst_fd), 16 - dst_fd.bitwidth);
    } else {
        res64 |= MASK_AT(value64, MASK_LOW(dst_fd), 0);
        res64 |= MASK_AT(value64, MASK_MID(dst_fd), dst_fd.bitoffset);
        res64 |= MASK_AT(value64, MASK_TOP(dst_fd), dst_fd.bytecount * 8 - dst_fd.bitwidth);
    }
    memcpy(dst_fd.byte_addr, &res64, dst_fd.bytecount);
}

uint64_t GET_INT64_AUTO(bitfield_handle_t fd) {
    return fd.meta ? GET_INT64_AUTO_META(fd) : GET_INT64_AUTO_NON_META(fd)
}

uint64_t GET_INT64_AUTO_META(bitfield_handle_t fd) {
    if (fd.bytecount == 1)
        return FLD_MASKED_BYTES(fd) >> (8 - fd.bitcount);

    return (FLD_BYTES(fd) & MASK_LOW(fd)) |
            ((FLD_BYTES(fd) & MASK_MID(fd)) >> fd.bitoffset) |
            ((FLD_BYTES(fd) & MASK_TOP(fd)) >> (fd.bytecount * 8 - fd.bitwidth));
}

uint64_t GET_INT64_AUTO_NON_META(bitfield_handle_t fd) {
    if (fd.bytecount == 1)
        return FLD_MASKED_BYTES(fd) >> (8 - fd.bitcount);
    
    if (fd.bytecount == 2)
        return rte_be_to_cpu_16(FLD_MASKED_BYTES(fd)) >> (16 - fd.bitcount);

    if (fd.bytecount <= 4)
        return rte_be_to_cpu_32(FLD_MASKED_BYTES(fd)) >> (32 - fd.bitcount);

    return rte_be_to_cpu_64(FLD_MASKED_BYTES(fd)) >> (64 - fd.bitcount);
}

void set_field(fldT f[], bufT b[], uint64_t value64, int bit_width) {
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
              value64,
              2 * byte_width,
              value64);

        MODIFY_INT64_INT64_AUTO_PACKET(fld.pd, fld.hdr, fld.fld, value64);
    }

    // TODO implement this case, too
    if (b != 0)   rte_exit(2, "TODO unimplemented portion of set_field");
}
