#include <stdio.h>
#include "util_packet.h"
#include <zlib.h>


uint8_t * hash_identity(struct uint8_buffer_s data, int size){
	debug("   :: Hashed with Identity\n");
	static uint8_t result[sizeof(size)];
	for(int i = 0; i < size; i++) {
		result[i] = data.buffer[i];
	}
	return &result;
}

uint8_t * hash_crc32(struct uint8_buffer_s data, int size){
	debug("   :: Hashed with CRC32\n");
	static uint8_t result[sizeof(size)];
	uLong crc = crc32(0L, Z_NULL, 0);
	for (int i = 0; i < data.buffer_size; i++) {
		crc = crc32(crc, &data.buffer[i], 1);
	}
	for (int i = 0; i<size; i++) {
		result[i] = (crc>>(i%sizeof(crc))*8)&0xFF;
	}
	return &result;
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


void hash(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t base, struct uint8_buffer_s data, uint32_t max, SHORT_STDPARAMS){
	uint8_t* hash_value;
	if (max > 0) {
		switch(algorithm) {
			case enum_HashAlgorithm_crc32:
//				hash_value = hash_crc32(data, hash_length);
				memcpy(hash_start, hash_crc32(data, hash_length), hash_length);
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
//				hash_value = hash_identity(data, hash_length);
				memcpy(hash_start, hash_identity(data, hash_length), hash_length);
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
//				for (int i = 0; i< hash_length; i++) {
//					hash_value[i] = 0;
//				}
				debug("   :: Invalid hash method chosen");
				break;
		}
	}else{
//		for (int i = 0; i< hash_length; i++) {
//			hash_value[i] = 0;
//		}
	}

}
