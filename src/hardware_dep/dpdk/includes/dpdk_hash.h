#include <stdio.h>
#include "util_packet.h"
#include <zlib.h>
#include <inttypes.h>

void hash_identity(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashed with Identity\n");
	for(int i = 0; i < size; i++) {
		hash_value[size-1-i] = data.buffer[i%data.buffer_size];
	}
}

void hash_crc32(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashed with CRC32\n");
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, data.buffer, data.buffer_size);
	if (sizeof(crc) < size) {size = sizeof(crc);}

	memcpy(hash_value, &crc, size);
}

void hash_crc32_custom(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of CRC32-Custom\n");
}

void hash_crc16(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of CRC16\n");
}

void hash_crc16_custom(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of CRC16-Custom\n");
}

void hash_csum16(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of CSUM16\n");
}

void hash_random(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of RANDOM\n");
}

void hash_xor16(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Simulated hashing of COR16\n");
}


void hash(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t base, struct uint8_buffer_s data, uint32_t max, SHORT_STDPARAMS){
	uint8_t* hash_value;
	if (max > 0) {
		switch(algorithm) {
			case enum_HashAlgorithm_crc32:
				hash_crc32(hash_start, data, hash_length);
				break;
			case enum_HashAlgorithm_crc32_custom:
				hash_crc32_custom(hash_start, data, hash_length);
				sheep((uint32_t) 100, pd, tables);
				break;
			case enum_HashAlgorithm_crc16:
				hash_crc16(hash_start, data, hash_length);
				sheep((uint32_t) 1000, pd, tables);
				break;
			case enum_HashAlgorithm_crc16_custom:	
				hash_crc16_custom(hash_start, data, hash_length);
				sheep((uint32_t) 10000, pd, tables);
				break;
			case enum_HashAlgorithm_random:
				hash_random(hash_start, data, hash_length);
				sheep((uint32_t) 100, pd, tables);
				break;
			case enum_HashAlgorithm_identity:	
				hash_identity(hash_start, data, hash_length);
				break;
			case enum_HashAlgorithm_csum16:	
				hash_csum16(hash_start, data, hash_length);
				sheep((uint32_t) 100, pd, tables);
				break;
			case enum_HashAlgorithm_xor16:
				hash_xor16(hash_start, data, hash_length);
				sheep((uint32_t) 100, pd, tables);
				break;
			default:
				for (int i = 0; i<hash_length; i++) {
					hash_value[i] = 0xFF;
				}
				debug("   :: Invalid hash method chosen");
				break;
		}
	}else{
		//TODO: Base
		for (int i = 0; i< hash_length; i++) {
			hash_value[i] = 0;
		}
	}
#ifdef T4P4S_DEBUG
	switch(hash_length) {
		case 1:
			debug("    : Hashed to " T4LIT(%x) "\n", *hash_start);
			break;
		case 2:
			debug("    : Hashed to " T4LIT(%x) "\n", *(uint16_t*)hash_start);
			break;
		case 3:
		case 4:
			debug("    : Hashed to " T4LIT(%x) "\n", *(uint32_t*)hash_start);
			break;
		case 5:
			
		case 6:
		case 7:
		case 8:
			debug("    : Hashed to %" PRIX64 "\n", *(uint64_t*)hash_start);
			break;
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			debug("    : Hashed to %" PRIX64 " %" PRIX64 "\n", *(uint64_t*)(hash_start+8), *(uint64_t*)(hash_start));
			break;
		default:
			debug("    : Hashed to value > 128 bit\n");
	}
#endif

}
