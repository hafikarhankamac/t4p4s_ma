#include <stdio.h>
#include "util_packet.h"
#include <zlib.h>
#include <inttypes.h>

void hash_identity(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashed with Identity\n");
	int min = size < data.buffer_size ? size : data.buffer_size;
	memcpy(hash_value, data.buffer, min);
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
	debug("   :: Simulated hashing of XOR16\n");
}


void calculate_hash(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, struct uint8_buffer_s data, SHORT_STDPARAMS){
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
				hash_start[i] = 0xFF;
			}
			debug("   :: Invalid hash method chosen");
			break;
	}
}


void hash_debug(uint8_t* hash_start, int hash_length){
	switch(hash_length) {
		case 0:
			debug("    : Not hashed (invalid length of 0)\n");
			break;
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
			debug("    : Hashed to %" PRIX64 "%" PRIX64 "\n", *(uint64_t*)(hash_start+8), *(uint64_t*)(hash_start));
			break;
		default:
			debug("    : Hashed to value > 128 bit\n");
	}
}

/*
void apply_borders(uint8_t* hash_start, int hash_length, uint8_t* base_start, int base_length, uint8_t* max_start, int max_length){
	bool max_value_invalid = False;
	bool base_value_invalid = False;
	if (hash_length < max_length) {
		for (int i = hash_length; i<max_length; i++) {
			if (*(max_start+i) > 0){
				max_value_invalid = True;
				break;
			}
		}
	}
	if (hash_length < base_length) {
		for(int i = hash_length; i<base_length; i++){
			if(*(base_start+i) > 0){
				base_value_invalid = True;
				break;
			}
		}
	}
	
	if (max_value_invalid) {
		
	}


}
*/

void hash(uint8_t* hash_start, int hash_length, uint8_t* base_start, int base_length, uint8_t* max_start, int max_length, enum enum_HashAlgorithm algorithm, struct uint8_buffer_s data, SHORT_STDPARAMS){

//	int hash_length_adjusted = max_length < hash_length ? max_length : hash_length;

//	int hash_length_adjusted = max_length < base_length: base

//	memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted); 
	if (max > 0) {
		calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
//		*(uint32_t*)hash_start = (uint32_t) base + (*(uint32_t*)hash_start % max);
	}else{
//		*(uint32_t*)hash_start = (uint32_t) base;
	}
#ifdef T4P4S_DEBUG
	hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

/*
void hash_b8_m8(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint8_t base, struct uint8_buffer_s data, uint8_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint8_t*)hash_start = (uint8_t) base + (*(uint8_t*)hash_start % max);
    }else{
        *(uint8_t*)hash_start = (uint8_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

void hash_b8_m16(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint8_t base, struct uint8_buffer_s data, uint16_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint8_t*)hash_start = (uint8_t) base + (*(uint8_t*)hash_start % max);
    }else{
        *(uint8_t*)hash_start = (uint8_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

void hash_b8_m32(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint8_t base, struct uint8_buffer_s data, uint32_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint8_t*)hash_start = (uint8_t) base + (*(uint8_t*)hash_start % max);
    }else{
        *(uint8_t*)hash_start = (uint8_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

void hash_b16_m8(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t base, struct uint8_buffer_s data, uint8_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint16_t*)hash_start = (uint16_t) base + (*(uint16_t*)hash_start % max);
    }else{
        *(uint16_t*)hash_start = (uint16_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

void hash_b16_m16(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t base, struct uint8_buffer_s data, uint16_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint16_t*)hash_start = (uint16_t) base + (*(uint16_t*)hash_start % max);
    }else{
        *(uint16_t*)hash_start = (uint16_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

void hash_b16_m32(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t base, struct uint8_buffer_s data, uint32_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint16_t*)hash_start = (uint16_t) base + (*(uint16_t*)hash_start % max);
    }else{
        *(uint16_t*)hash_start = (uint16_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

void hash_b32_m8(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint32_t base, struct uint8_buffer_s data, uint8_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint32_t*)hash_start = (uint32_t) base + (*(uint32_t*)hash_start % max);
    }else{
        *(uint32_t*)hash_start = (uint32_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

void hash_b32_m16(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint32_t base, struct uint8_buffer_s data, uint16_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint32_t*)hash_start = (uint32_t) base + (*(uint32_t*)hash_start % max);
    }else{
        *(uint32_t*)hash_start = (uint32_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}

void hash_b32_m32(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint32_t base, struct uint8_buffer_s data, uint32_t max, SHORT_STDPARAMS){
    int hash_length_adjusted = sizeof(max) < hash_length ? sizeof(max) : hash_length;
    memset(hash_start+hash_length_adjusted,0,hash_length-hash_length_adjusted);
    if (max > 0) {
        calculate_hash(hash_start, hash_length_adjusted, algorithm, data, SHORT_STDPARAMS_IN);
        *(uint32_t*)hash_start = (uint32_t) base + (*(uint32_t*)hash_start % max);
    }else{
        *(uint32_t*)hash_start = (uint32_t) base;
    }
#ifdef T4P4S_DEBUG
    hash_debug((uint8_t*)hash_start, hash_length);
#endif
}
*/
