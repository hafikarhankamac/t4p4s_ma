#ifndef __BIGNUM_H__
#define __BIGNUM_H__
/*

Big number library - arithmetic on multiple-precision unsigned integers.

This library is an implementation of arithmetic on arbitrarily large integers.

The difference between this and other implementations, is that the data structure
has optimal memory utilization (i.e. a 1024 bit integer takes up 128 bytes RAM),
and all memory is allocated statically: no dynamic allocation for better or worse.

Primary goals are correctness, clarity of code and clean, portable implementation.
Secondary goal is a memory footprint small enough to make it suitable for use in
embedded applications.


The current state is correct functionality and adequate performance.
There may well be room for performance-optimizations and improvements.

*/

#include <stdint.h>
#include <assert.h>

/* Helper macros */
#define BITS_TO_BYTES(bits) ((bits + 7) / 8)
#define BITS_IN_FIRST_BYTE(bytes, bits) (8 - (bytes * 8 - bits))
#define BIT_MASK(bits) ((1 << bits) - 1)
#define GET_SIGN(arr, first_bits) (arr[0] >> (first_bits - 1))

#define SIGNED_TYPE int

/**
 * uint32_t if 32-bit version, uint64_t if 64-bit version
 */
#define UNSIGNED_TYPE uint32_t

#define BIGNUM_FROM_INT(n, i, length_in_bytes) _Generic((i), \
  UNSIGNED_TYPE: bignum_from_int_unsigned, \
  SIGNED_TYPE: bignum_from_int_signed \
)(n, i, length_in_bytes)

/**
 * Creates an array from an unsigned integers
 */
void bignum_from_int_unsigned(uint8_t* dst, UNSIGNED_TYPE src, uint8_t length_in_bytes);

/**
 * Creates an array from a signed integer
 */
void bignum_from_int_signed(uint8_t* dst, SIGNED_TYPE src, uint8_t length_in_bytes);

/**
 * Creates an unsigned integer from an (unsigned) array
 */
UNSIGNED_TYPE bignum_to_int(uint8_t* src, uint8_t length_in_bytes);

/**
 * Creates a signed integer from an (signed) array
 */
SIGNED_TYPE bignum_to_int_signed(uint8_t* src, uint8_t length_in_bytes);

/**
 * Moves a value from an array into an array of different size (unsigned).
 */
void bignum_cast(uint8_t* src, uint8_t length_src_in_bits, uint8_t* dst, uint8_t length_dest_in_bits);

/**
 * Moves a value from an array into an array of different size while maintaining the sign.
 */
void bignum_cast_signed(uint8_t* src, uint8_t length_src_in_bits, uint8_t* dst, uint8_t length_dest_in_bits);

/**
 * Selects the correct concatenation function based on the inputs
 */
#define BIGNUM_CONCAT(a, la, b, lb, c) _Generic((a), \
  uint8_t*: _Generic((b), \
    uint8_t*: bignum_concat_arr_arr, \
    UNSIGNED_TYPE: bignum_concat_arr_int), \
  UNSIGNED_TYPE: _Generic((b), \
    uint8_t*: bignum_concat_int_arr,\
    UNSIGNED_TYPE: bignum_concat_int_int \
  ))(a, la, b, lb, c)

/**
 * Cocnatenates two arrays (length of dst array is implicitly given)
 */
void bignum_concat_arr_arr(uint8_t* left, uint8_t length_left_in_bits, uint8_t* right, uint8_t length_right_in_bits, uint8_t* dst);

/**
 * Cocnatenates an array and an integer (length of dst array is implicitly given)
 */
void bignum_concat_arr_int(uint8_t* left, uint8_t length_left_in_bits, UNSIGNED_TYPE right, uint8_t length_right_in_bits, uint8_t* dst);

/**
 * Cocnatenates an integer and an array (length of dst array is implicitly given)
 */
void bignum_concat_int_arr(UNSIGNED_TYPE left, uint8_t length_left_in_bits, uint8_t* right, uint8_t length_right_in_bits, uint8_t* dst);

/**
 * Cocnatenates two integers into an array (length of dst array is implicitly given)
 */
void bignum_concat_int_int(UNSIGNED_TYPE left, uint8_t length_left_in_bits, UNSIGNED_TYPE right, uint8_t length_right_in_bits, uint8_t* dst);

void _lshift_one_bit(uint8_t* a, uint8_t length_in_bytes);
void _rshift_one_bit(uint8_t* a, uint8_t length_in_bytes);

void _rshift_word_signed(uint8_t* a, int nwords, uint8_t length_in_bits);

/**
 * Sets unused bits to zero
 */
void bignum_truncate(uint8_t* target, uint8_t remaining_bits);

/**
 * Extracts a slice from an array into another array
 */
void bignum_slice(uint8_t* src, uint8_t length_src_in_bits, uint8_t* dst, uint8_t from, uint8_t to);

/**
 * Extracts a slice from an array into an integer
 */
UNSIGNED_TYPE bignum_slice_int(uint8_t* src, uint8_t length_in_bits, uint8_t from, uint8_t to);

/**
 * Unsigned addition of two arrays
 */
void bignum_add(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * Unsigned subtraction of two arrays
 */
void bignum_sub(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * Unsigned multiplication of two arrays
 */
void bignum_mul(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * Signed addition of two arrays
 */
void bignum_add_signed(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * Signed subtraction of two arrays
 */
void bignum_sub_signed(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * Signed multiplication of two arrays
 */
void bignum_mul_signed(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * Turns a negative value positive (assumes that input is negative)
 */
void bignum_make_positive(uint8_t* src, uint8_t* dst, uint8_t length_in_bits);

/**
 * Turns a positive value negative (assumes that input is positive)
 */
void bignum_make_negative(uint8_t* src, uint8_t* dst, uint8_t length_in_bits);

/**
 * Negates an array
 */
void bignum_negate(uint8_t* src, uint8_t* dst, uint8_t length_in_bits);

/**
 * AND of two arrays
 */
void bignum_and(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * OR of two arrays
 */
void bignum_or(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * XOR of two arrays
 */
void bignum_xor(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits);

/**
 * Complement of an array
 */
void bignum_not(uint8_t* src, uint8_t* dst, uint8_t length_in_bits);

/**
 * Unsigned left shift by nbits
 */
void bignum_lshift(uint8_t* src, uint8_t* dst, int nbits, uint8_t length_in_bits);

/**
 * Unsigned right shift by nbits
 */
void bignum_rshift(uint8_t* src, uint8_t* dst, int nbits, uint8_t length_in_bits);

/**
 * Signed left shift by nbits
 */
void bignum_lshift_signed(uint8_t* src, uint8_t* dst, int nbits, uint8_t length_in_bits);

/**
 * Signed right shift by nbits
 */
void bignum_rshift_signed(uint8_t* src, uint8_t* dst, int nbits, uint8_t length_in_bits);

/**
 * Signed equality comparison of two arrays
 */
int bignum_cmp_signed(uint8_t* left, uint8_t* right, uint8_t length_in_bits);

/**
 * Unsigned left shift by nbits
 */
void bignum_assign(uint8_t* dst, uint8_t* src, uint8_t length_in_bytes);


#endif /* #ifndef __BIGNUM_H__ */


