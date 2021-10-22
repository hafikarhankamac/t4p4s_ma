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


/* This macro defines the word size in bytes of the array that constitues the big-number data structure. */
#ifndef WORD_SIZE
  #define WORD_SIZE 1
#endif


/* Here comes the compile-time specialization for how large the underlying array size should be. */
/* The choices are 1, 2 and 4 bytes in size with uint32, uint64 for WORD_SIZE==4, as temporary. */
#ifndef WORD_SIZE
  #error Must define WORD_SIZE to be 1, 2, 4
#elif (WORD_SIZE == 1)
  /* Data type of array in structure */
  #define DTYPE                    uint8_t
  /* bitmask for getting MSB */
  #define DTYPE_MSB                ((DTYPE_TMP)(0x80))
  /* Data-type larger than DTYPE, for holding intermediate results of calculations */
  #define DTYPE_TMP                uint32_t
  /* sprintf format string */
  #define SPRINTF_FORMAT_STR       "%.02x"
  #define SSCANF_FORMAT_STR        "%2hhx"
  /* Max value of integer type */
  #define MAX_VAL                  ((DTYPE_TMP)0xFF)
#endif
#ifndef DTYPE
  #error DTYPE must be defined to uint8_t, uint16_t uint32_t or whatever
#endif


/* Custom assert macro - easy to disable */
#define require(p, msg) assert(p && msg)

/* Helper macros */
#define BITS_TO_BYTES(bits) (bits + 7) / 8
#define BITS_IN_FIRST_BYTE(bytes, bits) 8 - (bytes * 8 - bits)
#define BIT_MASK(bits) ((1 << bits) - 1)
#define GET_SIGN(arr, first_bits) (arr[0] >> (first_bits - 1))

/* Tokens returned by bignum_cmp() for value comparison */
enum { SMALLER = -1, EQUAL = 0, LARGER = 1 };

/* Initialization functions: */
// void bignum_init(struct bn* n);
void bignum_from_int(uint8_t* n, uint32_t i, uint8_t length_in_bytes);
void bignum_from_int_signed(uint8_t* n, DTYPE_TMP i, uint8_t length_in_bytes);
DTYPE_TMP bignum_to_int(uint8_t* n, uint8_t length);
uint32_t bignum_to_int_signed(uint8_t* n, uint8_t length);
void bignum_cast(uint8_t* a, uint8_t length_a_in_bits, uint8_t* b, uint8_t length_b_in_bits);
void bignum_cast_signed(uint8_t* a, uint8_t length_a, uint8_t* b, uint8_t length_b);

#define bignum_concat(a, la, b, lb, c) _Generic((a), \
  uint8_t*: _Generic((b), \
    uint8_t*: bignum_concat_arr_arr, \
    uint32_t: bignum_concat_arr_int), \
  uint32_t: _Generic((b), \
    uint8_t*: bignum_concat_int_arr,\
    uint32_t: bignum_concat_int_int \
  ))(a, la, b, lb, c)

void _lshift_one_bit(uint8_t* a, uint8_t length);
void _rshift_one_bit(uint8_t* a, uint8_t length);

void _rshift_word_signed(uint8_t* a, int nwords, uint8_t length_in_bits);

void bignum_concat_arr_arr(uint8_t* a, uint8_t length_a, uint8_t* b, uint8_t length_b, uint8_t* c);
void bignum_concat_arr_int(uint8_t* a, uint8_t length_a, uint32_t b, uint8_t length_b, uint8_t* c);
void bignum_concat_int_arr(uint32_t a, uint8_t length_a, uint8_t* b, uint8_t length_b, uint8_t* c);
void bignum_concat_int_int(uint32_t a, uint8_t length_a, uint32_t b, uint8_t length_b, uint8_t* c);

void bignum_truncate(uint8_t* a, uint8_t remaining_bits);

void bignum_slice(uint8_t* a, uint8_t length_in_bits, uint8_t* c, uint8_t from, uint8_t to);
uint32_t bignum_slice_int(uint8_t* a, uint8_t length_in_bits, uint8_t from, uint8_t to);

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
void bignum_inc(uint8_t* n, uint8_t length);                             /* Increment: add one to n */
void bignum_dec(uint8_t* n, uint8_t length);                             /* Decrement: subtract one from n */
// void bignum_pow(struct bn* a, struct bn* b, struct bn* c); /* Calculate a^b -- e.g. 2^10 => 1024 */
// void bignum_isqrt(struct bn* a, struct bn* b);             /* Integer square root -- e.g. isqrt(5) => 2*/
void bignum_assign(uint8_t* dst, uint8_t* src, uint8_t length);        /* Copy src into dst -- dst := src */


#endif /* #ifndef __BIGNUM_H__ */


