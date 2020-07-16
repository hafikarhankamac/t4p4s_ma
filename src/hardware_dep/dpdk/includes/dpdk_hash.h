//#include "util.h"

#ifndef DPDK_HASH_H
#define DPDK_HASH_H

typedef struct ipv4_5_tuple_t {
	uint32_t srcAddr;
	uint32_t dstAddr;
	uint8_t  prot;
	uint16_t srcPort;
	uint16_t dstPort;
} ipv4_5_tuple_t;

// Hash functions
void hash(uint8_t* hash_result, enum enum_HashAlgorithm algorithm, uint8_t base, ipv4_5_tuple_t data, uint8_t max);
#endif
