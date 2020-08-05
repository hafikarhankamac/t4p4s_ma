//#include "dpdk_hash.h"
#include "dpdk_lib.h"
#include "util.h"

//void hash(uint8_t* hash_result, enum enum_HashAlgorithm algorithm, uint8_t base, ipv4_5_tuple_s data, uint8_t max) {
//	debug("    : Hashing(" T4LIT(%s) "," T4L1T(%d) ",(" T4L1T(%d) "/" T4L1T(%d) "/" T4L1T(%d) "/" T4L1T(%d) "/" T4L1T(%d) ")," T4L1T(%d) " to " T4L1T(%d) "\n",
//		algorithm, base, ipv4_5_tuple->srcAddr, ipv4_5_tuple->dstAddr, ipv4_5_tuple->prot, ipv4_5_tuple->srcPort, ipv4_5_tuple->dstPort, max, hash_result);
//}

void hash(uint32_t* hash_result, enum enum_HashAlgorithm algorithm, uint16_t base, ipv4_5_tuple_s data, uint32_t max) {
//	debug("    : Hashing(" T4LIT(%s) "," T4L1T(%d) ",(" T4L1T(%d) "/" T4L1T(%d) "/" T4L1T(%d) "/" T4L1T(%d) "/" T4L1T(%d) ")," T4L1T(%d) " to " T4L1T(%d) "\n",
//		algorithm, base, ipv4_5_tuple->srcAddr, ipv4_5_tuple->dstAddr, ipv4_5_tuple->prot, ipv4_5_tuple->srcPort, ipv4_5_tuple->dstPort, max, hash_result);
}

