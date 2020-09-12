#include <stdio.h>
#include "util_packet.h"
#include <zlib.h>


uint8_t * hash_identity(struct uint8_buffer_s data, int size){
	debug("   :: Hashed with Identity\n");
	uint8_t result[size];
	for(int i = 0; i < size; i++) {
		result[i] = data.buffer[i];
	}
	return result;
}

uint8_t * hash_crc32(struct uint8_buffer_s data, int size){
	uint8_t result[size];
	uLong crc = crc32(0L, Z_NULL, 0);
	for (int i = 0; i < data.buffer_size; i++) {
		crc = crc32(crc, &data.buffer[i], 1);
	}
	for (int i = 0; i<size; i++) {
		result[i] = (crc>>(i%sizeof(crc))*8)&0xFF;
	}
	return result;
}

uint8_t * hash_crc32_custom(struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of CRC32-Custom\n");
	return 0;
}

uint8_t * hash_crc16(struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of CRC16\n");
	return 0;
}

uint8_t * hash_crc16_custom(struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of CRC16-Custom\n");
	return 0;
}

uint8_t * hash_csum16(struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of CSUM16\n");
	return 0;
}

uint8_t * hash_random(struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of RANDOM\n");
	return 0;
}

uint8_t * hash_xor16(struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of COR16\n");
	return 0;
}


void create_uint32_hash(uint32_t* hash_result, enum enum_HashAlgorithm algorithm, struct uint8_buffer_s data){
	int size = sizeof(uint32_t);
	uint8_t* result;
	switch(algorithm) {
		case enum_HashAlgorithm_crc32:
			result = hash_crc32(data, size);
			break;
		case enum_HashAlgorithm_crc32_custom:
			//memcpy(hash_result, &(hash_crc32_custom(data, size), size);
			//sheep((uint32_t) 100, pd, tables);
			break;
		case enum_HashAlgorithm_crc16:
			//memcpy(hash_result, &(hash_crc16(data, size), size);
			//sheep((uint32_t) 1000, pd, tables);
			break;
		case enum_HashAlgorithm_crc16_custom:	
			//memcpy(hash_result, &(hash_crc16_custom(data, size), size);
			//sheep((uint32_t) 10000, pd, tables);
			break;
		case enum_HashAlgorithm_random:
			//memcpy(hash_result, &(hash_random(data, size), size);
			//sheep((uint32_t) 100, pd, tables);
			break;
		case enum_HashAlgorithm_identity:	
			result = hash_identity(data,size);
			break;
		case enum_HashAlgorithm_csum16:	
			//memcpy(hash_result, &(hash_csum16(data, size), size);
			//sheep((uint32_t) 100, pd, tables);
			break;
		case enum_HashAlgorithm_xor16:
			//memcpy(hash_result, &(hash_xor16(data, size), size);
			//sheep((uint32_t) 100, pd, tables);
			break;
		default:
			result = 0;
			debug("   :: Invalid hash method chosen");
			break;
	}
	memcpy(&hash_result, &result, size);
}

void hash_r32_b16_m32(uint32_t* hash_result, enum enum_HashAlgorithm algorithm, uint16_t base, struct uint8_buffer_s data, uint32_t max, SHORT_STDPARAMS) {
	if (max > 0) {
		create_uint32_hash(hash_result, algorithm, data);
		hash_result = base + (uint32_t) hash_result % max;
	}else{
		hash_result = 0;
	}
	debug("    : Hashed to " T4LIT(%u) "\n", hash_result);
}


