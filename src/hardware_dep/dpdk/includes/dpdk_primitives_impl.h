// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

// this file is directly included from dpdk_primitives.h, no need for a "#pragma once"

#include <rte_byteorder.h>
#include "dataplane.h"
#include "util_packet.h"

#ifdef T4P4S_DEBUG
    #include <assert.h>
#endif

/*******************************************************************************
   Auxiliary
*******************************************************************************/

#define FLDINFOS(fld) (hdr_infos[fld_infos[fld].header_instance])

#define FLD_IS_FIXED_WIDTH(fld) (fld != FLDINFOS(fld).var_width_field)
#define FLD_IS_FIXED_POS(fld)   (FLDINFOS(fld).var_width_field == -1 || fld <= FLDINFOS(fld).var_width_field)

#define FLD_BITWIDTH(hdesc, fld) (FLD_IS_FIXED_WIDTH(fld) ? fld_infos[fld].bit_width : hdesc.var_width_field_bitwidth)
#define FLD_BYTEOFFSET(hdesc, fld) (fld_infos[fld].byte_offset + (FLD_IS_FIXED_POS(fld) ? 0 : (hdesc.var_width_field_bitwidth / 8)))

#define handle(hdesc, fld) \
        ((bitfield_handle_t) \
        { \
            .byte_addr   = (((uint8_t*)hdesc.pointer)+(FLD_BYTEOFFSET(hdesc, fld))), \
            .meta        = FLDINFOS(fld).is_metadata, \
            .bitwidth    = FLD_BITWIDTH(hdesc, fld), \
            .bytewidth   = (FLD_BITWIDTH(hdesc, fld) + 7) / 8, \
            .bitcount    = FLD_BITWIDTH(hdesc, fld) + fld_infos[fld].bit_offset, /* bitwidth + bitoffset */ \
            .bytecount   = ((FLD_BITWIDTH(hdesc, fld) + 7 + fld_infos[fld].bit_offset) / 8), \
            .bitoffset   = fld_infos[fld].bit_offset, \
            .byteoffset  = FLD_BYTEOFFSET(hdesc, fld), \
            .mask        = fld_infos[fld].mask, \
            .fixed_width = FLD_IS_FIXED_WIDTH(fld), \
        })

#define header_desc_buf(buf, w) ((header_descriptor_t) { -1, buf, -1, w })
#define header_desc_ins(pd, h)  ((pd)->headers[h])

/******************************************************************************/

#define FLD_MASK(fd) (fd.fixed_width ? fd.mask : \
    rte_cpu_to_be_64((~0UL << (64 - fd.bitcount)) & (~0UL >> fd.bitoffset)))

/* Casts */
#define FLD_BYTES(fd) (  fd.bytecount == 1 ? (*(uint8_t*)  fd.byte_addr) : \
                         ( fd.bytecount == 2 ? (*(uint16_t*) fd.byte_addr) : \
                         ( fd.bytecount <= 4 ? (*(uint32_t*) fd.byte_addr) : \
                                                (*(uint64_t*) fd.byte_addr))) )

#define FLD_MASKED_BYTES(fd) (FLD_BYTES(fd) & FLD_MASK(fd))

#define BYTECOUNT(fd)  ((fd.bitcount - 1) / 8)

#define MASK_LOW(fd) (FLD_MASK(fd) & 0xffUL) // Gets the lowest byte
#define MASK_MID(fd) (FLD_MASK(fd) & (~0UL >> ((8 - BYTECOUNT(fd)) * 8)) & ~0xffUL) // Gets the middle bytes
#define MASK_TOP(fd) (FLD_MASK(fd) & (0xffUL << (BYTECOUNT(fd) * 8))) // Gets the highest byte depending on the size

/*******************************************************************************
   Modify - statement - bytebuf
*******************************************************************************/

void MODIFY_BYTEBUF_BYTEBUF(bitfield_handle_t dst_fd, uint8_t* src, uint8_t srclen);

/*******************************************************************************
   Modify - statement - int64
*******************************************************************************/

// Modifies a field in the packet by the given source and length (byte conversion when necessary) [MAX 4 BYTES]
// assuming `uint64_t value64' is in the scope
#define MODIFY_INT64_BYTEBUF(dst_fd, src, srclen) { \
    value64 = 0; \
    memcpy(&value64, src, srclen); \
    MODIFY_INT64_INT64_AUTO(dst_fd, value64); \
}

#define MASK_AT(value64,mask,bitcount) ((value64 & ((mask) >> (bitcount))) << (bitcount))

// Modifies a field in the packet by a uint64_t value (no byteorder conversion) [MAX 4 BYTES]
// assuming `uint64_t res64' is in the scope
#define MODIFY_INT64_INT64_BITS(dst_fd, value64) { \
    { \
        uint64_t res64 = (FLD_BYTES(dst_fd) & ~FLD_MASK(dst_fd)); \
        if (dst_fd.bytecount == 1) { \
            res64 |= (value64 << (8 - dst_fd.bitcount) & FLD_MASK(dst_fd)); \
        } else if (dst_fd.bytecount == 2) { \
            res64 |= MASK_AT(value64, MASK_LOW(dst_fd), 0); \
            res64 |= MASK_AT(value64, MASK_TOP(dst_fd), 16 - dst_fd.bitwidth); \
        } else { \
            res64 |= MASK_AT(value64, MASK_LOW(dst_fd), 0); \
            res64 |= MASK_AT(value64, MASK_MID(dst_fd), dst_fd.bitoffset); \
            res64 |= MASK_AT(value64, MASK_TOP(dst_fd), dst_fd.bytecount * 8 - dst_fd.bitwidth); \
        } \
        memcpy(dst_fd.byte_addr, &res64, dst_fd.bytecount); \
    } \
}

// Modifies a field in the packet by a uint64_t value with byte conversion (always) [MAX 4 BYTES]
// assuming `uint64_t res64' is in the scope
#define MODIFY_INT64_INT64_HTON(dst_fd, value64) { \
    { \
        uint64_t res64 = (FLD_BYTES(dst_fd) & ~FLD_MASK(dst_fd)); \
        if (dst_fd.bytecount == 1) \
            res64 |= (value64 << (8 - dst_fd.bitcount)) & FLD_MASK(dst_fd); \
        else if (dst_fd.bytecount == 2) \
            res64 |= rte_cpu_to_be_16(value64 << (16 - dst_fd.bitcount)) & FLD_MASK(dst_fd); \
        else if (dst_fd.bytecount <= 4) \
            res64 |= rte_cpu_to_be_32(value64 << (32 - dst_fd.bitcount)) & FLD_MASK(dst_fd); \
        else \
            res64 |= rte_cpu_to_be_64(value64 << (64 - dst_fd.bitcount)) & FLD_MASK(dst_fd); \
        memcpy(dst_fd.byte_addr, &res64, dst_fd.bytecount); \
    } \
}

// Modifies a field in the packet by a uint64_t value with byte conversion when necessary [MAX 4 BYTES]
// assuming `uint64_t res64' is in the scope
#define MODIFY_INT64_INT64_AUTO(dst_fd, value64) { \
    if (dst_fd.meta) { MODIFY_INT64_INT64_BITS(dst_fd, value64) } else { MODIFY_INT64_INT64_HTON(dst_fd, value64) } \
}

/*******************************************************************************
   Extract - expression (unpack value and return it)
*******************************************************************************/

// Gets the value of a field

#define GET_INT64_AUTO(fd) (fd.meta ? GET_INT64_AUTO_META(fd) : GET_INT64_AUTO_NON_META(fd))

#define GET_INT64_AUTO_META(fd) (fd.bytecount == 1 ? (FLD_MASKED_BYTES(fd) >> (8 - fd.bitcount)) : \
                                        ((FLD_BYTES(fd) & MASK_LOW(fd)) | \
                                        ((FLD_BYTES(fd) & MASK_MID(fd)) >> fd.bitoffset) | \
                                        ((FLD_BYTES(fd) & MASK_TOP(fd)) >> (fd.bytecount * 8 - fd.bitwidth))))

#define GET_INT64_AUTO_NON_META(fd) \
    (fd.bytecount == 1 ? (FLD_MASKED_BYTES(fd) >> (8 - fd.bitcount)) : \
        (fd.bytecount == 2 ? (rte_be_to_cpu_16(FLD_MASKED_BYTES(fd)) >> (16 - fd.bitcount)) : \
            (fd.bytecount <= 4 ? (rte_be_to_cpu_32(FLD_MASKED_BYTES(fd)) >> (32 - fd.bitcount)) : \
                rte_be_to_cpu_64(FLD_MASKED_BYTES(fd)) >> (64 - fd.bitcount))))

/*******************************************************************************
   Extract - statement (unpack value to a destination variable)
*******************************************************************************/

// Extracts a field to the given uint64_t variable with byte conversion (always) [MAX 4 BYTES]
#define EXTRACT_INT64_NTOH(fd, dst) { \
    if (fd.bytecount == 1) \
        dst =                  FLD_MASKED_BYTES(fd) >> (8  - fd.bitcount); \
    else if (fd.bytecount == 2) \
        dst = rte_be_to_cpu_16(FLD_MASKED_BYTES(fd)) >> (16 - fd.bitcount); \
    else if (fd.bytecount <= 4) \
        dst = rte_be_to_cpu_32(FLD_MASKED_BYTES(fd)) >> (32 - fd.bitcount); \
    else \
        dst = rte_be_to_cpu_64(FLD_MASKED_BYTES(fd)) >> (64 - fd.bitcount); \
}

// Extracts a field to the given uint64_t variable (no byteorder conversion) [MAX 4 BYTES]
#define EXTRACT_INT64_BITS(fd, dst) { \
    if (fd.bytecount == 1) \
        dst = FLD_MASKED_BYTES(fd) >> (8 - fd.bitcount); \
    else if (fd.bytecount == 2) \
        dst = (FLD_BYTES(fd) & MASK_LOW(fd)) | \
             ((FLD_BYTES(fd) & MASK_TOP(fd)) >> (16 - fd.bitwidth)); \
    else \
        dst = (FLD_BYTES(fd) & MASK_LOW(fd)) | \
             ((FLD_BYTES(fd) & MASK_MID(fd)) >> fd.bitoffset) | \
             ((FLD_BYTES(fd) & MASK_TOP(fd)) >> (fd.bytecount * 8 - fd.bitwidth)); \
}

// Extracts a field to the given uint64_t variable with byte conversion when necessary [MAX 4 BYTES]
#define EXTRACT_INT64_AUTO(fd, dst) { \
    if (fd.meta) { EXTRACT_INT64_BITS(fd, dst) } else { EXTRACT_INT64_NTOH(fd, dst) } \
}

void EXTRACT_BYTEBUF(bitfield_handle_t fd, uint8_t* dst);


/*******************************************************************************/

void set_field(fldT f[], bufT b[], uint64_t value64, int bit_width);

void MODIFY_INT64_INT64_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t h, field_instance_t f, uint64_t value64);
