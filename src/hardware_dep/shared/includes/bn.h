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

/* Custom assert macro - easy to disable */
#define REQUIRE(p, msg) assert(p && msg)

/* Helper macros */
#define BITS_TO_BYTES(bits) ((bits + 7) / 8)
#define BITS_IN_FIRST_BYTE(bytes, bits) (8 - (bytes * 8 - bits))
#define BIT_MASK(bits) ((1 << bits) - 1)
#define GET_SIGN(arr, first_bits) (arr[0] >> (first_bits - 1))

/* Types */
#define SIGNED_TYPE int
#define UNSIGNED_TYPE uint32_t

/* Tokens returned by bignum_cmp() for value comparison */
enum { SMALLER = -1, EQUAL = 0, LARGER = 1 };

/* Initialization functions: */

#define BIGNUM_FROM_INT(n, i, length_in_bytes) _Generic((i), \
  UNSIGNED_TYPE: bignum_from_int_unsigned, \
  SIGNED_TYPE: bignum_from_int_signed \
)(n, i, length_in_bytes)

void bignum_from_int_unsigned(uint8_t* n, UNSIGNED_TYPE i, uint8_t length_in_bytes);
void bignum_from_int_signed(uint8_t* n, SIGNED_TYPE i, uint8_t length_in_bytes);

UNSIGNED_TYPE bignum_to_int(uint8_t* n, uint8_t length);
SIGNED_TYPE bignum_to_int_signed(uint8_t* n, uint8_t length);
void bignum_cast(uint8_t* a, uint8_t length_a_in_bits, uint8_t* b, uint8_t length_b_in_bits);
void bignum_cast_signed(uint8_t* a, uint8_t length_a, uint8_t* b, uint8_t length_b);

// Concatenation

#define BIGNUM_CONCAT(a, la, b, lb, c) _Generic((a), \
  uint8_t*: _Generic((b), \
    uint8_t*: bignum_concat_arr_arr, \
    UNSIGNED_TYPE: bignum_concat_arr_int), \
  UNSIGNED_TYPE: _Generic((b), \
    uint8_t*: bignum_concat_int_arr,\
    UNSIGNED_TYPE: bignum_concat_int_int \
  ))(a, la, b, lb, c)

void bignum_concat_arr_arr(uint8_t* a, uint8_t length_a, uint8_t* b, uint8_t length_b, uint8_t* c);
void bignum_concat_arr_int(uint8_t* a, uint8_t length_a, UNSIGNED_TYPE b, uint8_t length_b, uint8_t* c);
void bignum_concat_int_arr(UNSIGNED_TYPE a, uint8_t length_a, uint8_t* b, uint8_t length_b, uint8_t* c);
void bignum_concat_int_int(UNSIGNED_TYPE a, uint8_t length_a, UNSIGNED_TYPE b, uint8_t length_b, uint8_t* c);

void _lshift_one_bit(uint8_t* a, uint8_t length);
void _rshift_one_bit(uint8_t* a, uint8_t length);

void _rshift_word_signed(uint8_t* a, int nwords, uint8_t length_in_bits);

void bignum_truncate(uint8_t* a, uint8_t remaining_bits);

void bignum_slice(uint8_t* a, uint8_t length_in_bits, uint8_t* c, uint8_t from, uint8_t to);
UNSIGNED_TYPE bignum_slice_int(uint8_t* a, uint8_t length_in_bits, uint8_t from, uint8_t to);

/* Basic arithmetic operations: */
void bignum_add(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits); /* c = a + b */
void bignum_sub(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits); /* c = a - b */
void bignum_mul(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits); /* c = a * b */
void bignum_div(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits); /* c = a / b */
void bignum_mod(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits); /* c = a % b */
void bignum_divmod(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t* d, uint8_t length_in_bits); /* c = a/b, d = a%b */

void bignum_add_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits);
void bignum_sub_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits);
void bignum_mul_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits);
void bignum_div_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits);
void bignum_mod_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits);

void bignum_make_positive(uint8_t* a, uint8_t* b, uint8_t length);
/* Change sign of number */
void bignum_make_negative(uint8_t* a, uint8_t* b, uint8_t length);
void bignum_negate(uint8_t* a, uint8_t* b, uint8_t length_in_bits);

/* Bitwise operations: */
void bignum_and(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bytes); /* c = a & b */
void bignum_or(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bytes);  /* c = a | b */
void bignum_xor(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bytes); /* c = a ^ b */
void bignum_not(uint8_t* a, uint8_t* b, uint8_t length_in_bits);

/* Shifts */
void bignum_lshift(uint8_t* a, uint8_t* b, int nbits, uint8_t length_in_bits); /* b = a << nbits */
void bignum_rshift(uint8_t* a, uint8_t* b, int nbits, uint8_t length_in_bits); /* b = a >> nbits */
void bignum_lshift_signed(uint8_t* a, uint8_t* b, int nbits, uint8_t length_in_bytes); /* b = a << nbits */
void bignum_rshift_signed(uint8_t* a, uint8_t* b, int nbits, uint8_t length); /* b = a >> nbits */

/* Comparison */
int bignum_cmp(uint8_t* a, uint8_t* b, uint8_t length);             /* Compare: returns LARGER, EQUAL or SMALLER */
int bignum_cmp_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits);
int bignum_gr_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits);
int bignum_ge_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits);
int bignum_ls_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits);
int bignum_le_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits);

int  bignum_is_zero(uint8_t* n, uint8_t length);                         /* For comparison with zero */
// void bignum_pow(struct bn* a, struct bn* b, struct bn* c); /* Calculate a^b -- e.g. 2^10 => 1024 */
// void bignum_isqrt(struct bn* a, struct bn* b);             /* Integer square root -- e.g. isqrt(5) => 2*/
void bignum_assign(uint8_t* dst, uint8_t* src, uint8_t length);        /* Copy src into dst -- dst := src */


#endif /* #ifndef __BIGNUM_H__ */


