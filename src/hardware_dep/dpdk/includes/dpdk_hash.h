#include <stdio.h>
#include "util_packet.h"
#include <zlib.h>
#include <inttypes.h>


/* =====================================================================
				   Hash algorithms
   =====================================================================
 Here are the signatures for all hash algorithms that are currently contained
 in the T4P4S toolchain. See /path/to/p4c/p4include/v1model.p4 for used enums.
*/

void hash_identity(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashed with Identity\n");
	size = size < data.buffer_size ? size : data.buffer_size;

//	Reverse order
//	for(int i = 0; i < size; i++) {
//		debug("buffer %u\n", data.buffer[i%data.buffer_size]);
//		hash_value[size-1-i] = data.buffer[i%data.buffer_size];
//	}
	
	memcpy(hash_value, data.buffer, size);
}

void hash_crc32(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashed with CRC32\n");
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, data.buffer, data.buffer_size);

	if (sizeof(uLong) < size) {size = sizeof(uLong);}
	memcpy(hash_value, &crc, size);
}

void hash_crc32_custom(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashing with CRC32-Custom (not implemented yet)\n");
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc,data.buffer, data.buffer_size);
	for(int i = 0; i<size; i++){
		memcpy(hash_value+i, &crc+(i%sizeof(uLong)),1);
	}
//	memcpy(hash_value+size/sizeof(uLong), &crc, size%sizeof(uLong));
}

void hash_crc16(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashing with CRC16 (not implemented yet)\n");
}

void hash_crc16_custom(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashing with CRC16-Custom (not implemented yet)\n");
}

void hash_csum16(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashing with CSUM16 (not implemented yet)\n");
}

void hash_random(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashing with RANDOM (not implemented yet)\n");
}

void hash_xor16(uint8_t* hash_value, struct uint8_buffer_s data, int size){
	debug("   :: Hashing with XOR16 (not implemented yet)\n");
}



/**
 * Applies a hash algorithm
 * 
 * Chooses the respective hash algorithm to use and offers a place to use sheep (to simulate algorithms)
 *
 * @param hash_start: Pointer specifying the hash value's start byte
 * @param hash_length: Integer specifying the size of the hash
 * @param algorithm: Enum soecifying the hash algorithm to apply
 * @param data: Struct consisting of buffer and buffer_size specifying the data to hash
 * @param SHORT_STDPARAMS: Predefined parameters used for sheep
 *
 */
void calculate_hash(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, struct uint8_buffer_s data, SHORT_STDPARAMS){
	switch(algorithm) {
		case enum_HashAlgorithm_crc32:
			hash_crc32(hash_start, data, hash_length);
			break;
		case enum_HashAlgorithm_crc32_custom:
			hash_crc32_custom(hash_start, data, hash_length);
			//sheep((uint32_t) 100, pd, tables);
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

/**
 * Prints the hash value
 *
 * Prints the hash value with the debug functionality of t4p4s and uses the hash_length to
 * determine the fitting representation
 * 
 * @param hash_start: Pointer specifying the hash value's start byte
 * @param hash_length: Int specifying the size of the hash
 */
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

/* =====================================================================
				Hash signatures
   =====================================================================
 Here are the signatures for all hash paramterizations to avoid the use of macros
 and to enable flawless use of bit types 8, 16, 32 and 64 for the base and max
 parameters. Since only datatypes differ the following documentation summarizes
 all parameters
*/

/*
 * Hash signature hash_b{base_size}_m{max_size} for the respective bit sizes
 * 
 * The signature incorporates the used bit sizes to better distinguish them. The
 * first number specifies the bit size of the provided base value; the second number
 * specifies the bit size of the max value.
 * The implemented functionality provides the computation of a hash value in a specified
 * value space [base, base+max-1]. If the maximum value is 0, the returned value
 * will be equal to the base.
 * 
 * The long-term aim is to provide an entirely arbitrary parametrization. However,
 * the current state of t4p4s does not support greater values than 64bit to be
 * transformed from P4 code to C code (the variables themselves are transformed into adjusted
 * arrays, but the values do not exceed 64bit). Therefore, this implementation is focusing on
 * the datatypes 8, 16, 32 and 64 bit for base and max.
 *
 * The data to hash as well as the return value can be arbitrarily big. Thus, the return values
 * may be manipulated to represent greater value than 64bit ones by adjusting the corresponding
 * hash function.
 *
 * The current implementation does not interfere with the users parameters, since
 * error detection might be easier if the implementation does masquerade
 * unwanted behaviour by "optimizing" the users input. (a potential idea is to
 * adjust borders with a certain priority.)
 * 
 * By calculating the difference between actual hash length and border lengths, the hashing
 * can be reduced to the utilized parts. The known values are precalculated and represent
 * in total "min(hash_length, max(base_length, max_length))",
 * with base/max-length being the required bytes.
 * 
 * @param hash_start: Pointer specifying the hash value's start byte
 * @param hash_length: Int specifying the size of the hash value
 * @param algorithm:  Enum specifying the algorithm to hash
 * @param base: Pointer specifying the start byte of the base value
 * @param data: Struct consisting of buffer and buffer_size specifying the data to hash
 * @param max: Pointer specifying the start byte of the max value
 * @param SHORT_STDPARAMS: Predefined parameters used for sheep
 *
 */

/*
void hash_b8_m8(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint8_t* base, struct uint8_buffer_s data, uint8_t* max, SHORT_STDPARAMS){
    if (hash_length<=1){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 1 < hash_length ? 1 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint8_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b8_m16(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint8_t* base, struct uint8_buffer_s data, uint16_t* max, SHORT_STDPARAMS){
    if (hash_length<=1){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 2 < hash_length ? 2 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint8_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b8_m32(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint8_t* base, struct uint8_buffer_s data, uint32_t* max, SHORT_STDPARAMS){
    if (hash_length<=1){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 4 < hash_length ? 4 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint8_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b8_m64(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint8_t* base, struct uint8_buffer_s data, uint64_t* max, SHORT_STDPARAMS){
    if (hash_length<=1){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 8 < hash_length ? 8 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint8_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b16_m8(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t* base, struct uint8_buffer_s data, uint8_t* max, SHORT_STDPARAMS){
    if (hash_length<=2){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 2 < hash_length ? 2 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint16_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b16_m16(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t* base, struct uint8_buffer_s data, uint16_t* max, SHORT_STDPARAMS){
    if (hash_length<=2){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 2 < hash_length ? 2 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint16_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b16_m32(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t* base, struct uint8_buffer_s data, uint32_t* max, SHORT_STDPARAMS){
    if (hash_length<=2){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 4 < hash_length ? 4 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint16_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b16_m64(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint16_t* base, struct uint8_buffer_s data, uint64_t* max, SHORT_STDPARAMS){
    if (hash_length<=2){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 8 < hash_length ? 8 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint16_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b32_m8(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint32_t* base, struct uint8_buffer_s data, uint8_t* max, SHORT_STDPARAMS){
    if (hash_length<=4){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 4 < hash_length ? 4 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint32_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b32_m16(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint32_t* base, struct uint8_buffer_s data, uint16_t* max, SHORT_STDPARAMS){
    if (hash_length<=4){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 4 < hash_length ? 4 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint32_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}
*/
void hash_b32_m32(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint32_t base, struct uint8_buffer_s data, uint32_t max, SHORT_STDPARAMS){
    if (hash_length<=5){
          uint64_t hash_max_val = (1<<(hash_length*8))-1;
          if (hash_max_val-max < base){
               base = hash_max_val > max ? hash_max_val - max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 5 < hash_length ? 5 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
	if(base + (*(uint32_t*)hash_start) % max < base && hash_length>=5){
		hash_start[4] = 1;
	}
	*hash_start = base + (*(uint32_t*)hash_start) % max;
    }else{
        *hash_start = base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}
/*
void hash_b32_m64(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint32_t* base, struct uint8_buffer_s data, uint64_t* max, SHORT_STDPARAMS){
    if (hash_length<=4){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 8 < hash_length ? 8 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint32_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b64_m8(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint64_t* base, struct uint8_buffer_s data, uint8_t* max, SHORT_STDPARAMS){
    if (hash_length<=8){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 8 < hash_length ? 8 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint64_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b64_m16(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint64_t* base, struct uint8_buffer_s data, uint16_t* max, SHORT_STDPARAMS){
    if (hash_length<=8){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 8 < hash_length ? 8 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint64_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

void hash_b64_m32(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint64_t* base, struct uint8_buffer_s data, uint32_t* max, SHORT_STDPARAMS){
    if (hash_length<=8){
          uint64_t hash_max_val = 1<<(hash_length*8)-1;
          if (hash_max_val-max<base){
               base = hash_max_val > max ? hash_max_val-max : 0;
          }
    }
    if (max > 0) {
        calculate_hash(hash_start, 8 < hash_length ? 8 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint64_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}
*/
void hash_b64_m64(uint8_t* hash_start, int hash_length, enum enum_HashAlgorithm algorithm, uint64_t* base, struct uint8_buffer_s data, uint64_t* max, SHORT_STDPARAMS){
    if (hash_length<=8){
          uint64_t hash_max_val = (1<<(hash_length*8))-1;
          if (hash_max_val-*max<*base){
               *base = hash_max_val > *max ? hash_max_val-*max : 0;
          }
    }
    if (*max > 0) {
        calculate_hash(hash_start, 8 < hash_length ? 8 : hash_length, algorithm, data, SHORT_STDPARAMS_IN);
        *hash_start = *base + (*(uint64_t*)hash_start % *max);
    }else{
        *hash_start = *base;
    }
#ifdef T4P4S_DEBUG
    hash_debug(hash_start, hash_length);
#endif
}

