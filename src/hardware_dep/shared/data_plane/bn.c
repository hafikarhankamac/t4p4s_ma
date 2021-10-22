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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "bn.h"
#include <math.h>

#define MIN(a,b) a < b ? a : b

void _lshift_word(uint8_t* a, int nwords, uint8_t length);
void _rshift_word(uint8_t* a, int nwords, uint8_t length);

void bignum_make_positive(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t* tmp1 = malloc(bytes * sizeof(uint8_t));
  uint8_t* tmp2 = malloc(bytes * sizeof(uint8_t));
  memset(tmp1, 0, bytes * sizeof(uint8_t));

  bignum_from_int_signed(tmp2, 1, bytes);

  bignum_sub(a, tmp2, tmp1, length_in_bits);

  bignum_not(tmp1, b, length_in_bits);

  bignum_truncate(b, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
  free(tmp2);
  free(tmp1);
}

void bignum_make_negative(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t* tmp = malloc(bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, 1, bytes);

  bignum_not(a, b, length_in_bits);
  bignum_add(b, tmp, b, length_in_bits);

  bignum_truncate(b, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
  free(tmp);
}

void bignum_not(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t bits_in_first = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  for (int i = 0; i < bytes; i++) 
    b[i] = 0xff - a[i];

  bignum_truncate(b, bits_in_first);
}

void bignum_add_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits) {
  bignum_add(a, b, c, length_in_bits);
}

void bignum_sub_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);
  uint8_t sign = GET_SIGN(b, first_bits);

  if (sign) {
    uint8_t* tmp = malloc(bytes * sizeof(uint8_t));
    bignum_make_positive(b, tmp, length_in_bits);
    bignum_add(a, tmp, c, length_in_bits);
    free(tmp);
  }
  else {
    bignum_sub(a, b, c, length_in_bits);
  }
}

void bignum_mul_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  uint8_t isANeg = GET_SIGN(a, first_bits);
  uint8_t isBNeg = GET_SIGN(b, first_bits);;

    uint8_t *tmp1, *tmp2;

    if (isANeg) {
      tmp1 = malloc(bytes * sizeof(uint8_t));
      bignum_make_positive(a, tmp1, length_in_bits);
    }
    else
      tmp1 = a;

    if (isBNeg) {
      tmp2 = malloc(bytes * sizeof(uint8_t));
      bignum_make_positive(b, tmp2, length_in_bits);
    }
    else
      tmp2 = b;

    bignum_mul(tmp1, tmp2, c, length_in_bits);

    if (isANeg != isBNeg)
      bignum_make_negative(c, c, length_in_bits);

    if (isBNeg)
      free(tmp2);

    if (isANeg)
      free(tmp1);
}

void bignum_div_signed(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  uint8_t isANeg = GET_SIGN(a, first_bits);;
  uint8_t isBNeg = GET_SIGN(b, first_bits);;

  uint8_t *tmp1, *tmp2;

  if (isANeg) {
    tmp1 = malloc(bytes * sizeof(uint8_t));
    bignum_make_positive(a, tmp1, length_in_bits);
  }
  else
    tmp1 = a;

  if (isBNeg) {
    tmp2 = malloc(bytes * sizeof(uint8_t));
    bignum_make_positive(b, tmp2, length_in_bits);
  }
  else
    tmp2 = b;

  bignum_div(tmp1, tmp2, c, length_in_bits);

  if (isANeg != isBNeg)
    bignum_make_negative(c, c, length_in_bits);

  if (isBNeg)
    free(tmp2);

  if (isANeg)
    free(tmp1);
}

void bignum_from_int(uint8_t* n, uint32_t i, uint8_t length_in_bytes) {
  require(n, "n is null");
  memset(n, 0, length_in_bytes);

  for (int j = length_in_bytes - 1; j >= 0; j--) {
    uint32_t r = (length_in_bytes - 1 - j);
    uint32_t mask = ((uint32_t)0xff) << (r * 8);
    n[j] = (i & mask) >> r * 8;
  }
}

void bignum_from_int_signed(uint8_t* n, DTYPE_TMP i, uint8_t length_in_bytes)
{
  require(n, "n is null");
  memset(n, 0, length_in_bytes);

  for (int j = length_in_bytes - 1; j >= 0; j--) {
    int r = (length_in_bytes - 1 - j);
    DTYPE_TMP mask = ((DTYPE_TMP)0xff) << (r * 8);
    n[j] = (i & mask) >> r * 8;
  }
}

uint32_t bignum_to_int(uint8_t* n, uint8_t length) {
  uint8_t a = MIN(length, 4);

  uint32_t r = 0;
  for (int i = a - 1; i >= 0; i--) {
    uint32_t factor = (a - i - 1) * 8;
    r += (n[i] * (1 << factor));
  }

  return r;
}

DTYPE_TMP bignum_to_int_signed(uint8_t* n, uint8_t length) {
  if (!(n[0] >> 7))
    return bignum_to_int(n, length);

  uint8_t *temp = malloc(length * sizeof(uint8_t));
  uint8_t *temp2 = malloc(length * sizeof(uint8_t));
  temp2[length - 1] = 1;

  bignum_sub(n, temp2, temp, length*8);
  bignum_not(temp, temp, length*8);

  int r = bignum_to_int(temp, length);

  free(temp2);
  free(temp);
  return -r;
}

void bignum_cast(uint8_t* a, uint8_t length_a_in_bits, uint8_t* b, uint8_t length_b_in_bits) {
  int length_a_in_bytes = BITS_TO_BYTES(length_a_in_bits);
  int length_b_in_bytes = BITS_TO_BYTES(length_b_in_bits);

  uint8_t bits_in_first_b_byte = BITS_IN_FIRST_BYTE(length_b_in_bytes, length_b_in_bits);
  uint8_t mask = BIT_MASK(bits_in_first_b_byte);

  int min_length = MIN(length_a_in_bytes, length_b_in_bytes);

  for (int i = 0; i < min_length; i++)
    b[length_b_in_bytes - i - 1] = a[length_a_in_bytes - i - 1];

  b[length_b_in_bytes - min_length] &= mask;

  for (int i = 0; i < length_b_in_bytes - min_length; i++)
    b[i] = 0;
}

void bignum_cast_signed(uint8_t* a, uint8_t length_a_in_bits, uint8_t* b, uint8_t length_b_in_bits) {
  int length_a_in_bytes = BITS_TO_BYTES(length_a_in_bits);
  int length_b_in_bytes = BITS_TO_BYTES(length_b_in_bits);

  uint8_t bits_in_first_b_byte = BITS_IN_FIRST_BYTE(length_b_in_bytes, length_b_in_bits);
  uint8_t bits_in_first_a_byte = BITS_IN_FIRST_BYTE(length_a_in_bytes, length_a_in_bits);

  uint8_t sign = GET_SIGN(a, bits_in_first_a_byte);

  uint8_t min_length = MIN(length_a_in_bytes, length_b_in_bytes);

  for (int i = 0; i < min_length; i++)
    b[length_b_in_bytes - i - 1] = a[length_a_in_bytes - i - 1];

  if (length_b_in_bits > length_a_in_bits && sign) {
    uint8_t x = length_b_in_bytes > length_a_in_bytes ? 8 : bits_in_first_b_byte;
    uint8_t mask = BIT_MASK(x) ^ BIT_MASK(bits_in_first_a_byte);
    b[length_b_in_bytes - min_length] |= mask;
  }
  else {
    b[length_b_in_bytes - min_length] &= BIT_MASK(bits_in_first_b_byte);
  }

  for (int i = 0; i < length_b_in_bytes - min_length; i++)
    b[i] = sign ? 0xff : 0;
}

void bignum_dec(uint8_t* n, uint8_t length)
{
  require(n, "n is null");

  DTYPE tmp, res;
  int i;
  for (i = length - 1; i >= 0; --i)
  {
    tmp = n[i];
    res = tmp - 1;
    n[i] = res;

    if (!(res > tmp))
      break;
  }
}

void bignum_inc(uint8_t* n, uint8_t length)
{
  require(n, "n is null");

  DTYPE res;
  DTYPE_TMP tmp; /* copy of n */

  int i;
  for (i = length - 1; i >= 0; --i)
  {
    tmp = n[i];
    res = tmp + 1;
    n[i] = res;

    if (res > tmp)
      break;
  }
}

void bignum_add(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");

  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t bits_in_first_byte = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  DTYPE_TMP tmp;
  int carry = 0;
  for (int i = 0; i < bytes; ++i)
  {
    tmp = (DTYPE_TMP)a[bytes - 1 - i] + b[bytes - 1 - i] + carry;
    carry = (tmp > MAX_VAL);
    c[bytes - 1 - i] = (tmp & MAX_VAL);
  }

  bignum_truncate(c, bits_in_first_byte);
}

void bignum_sub(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);

  DTYPE_TMP res, tmp1, tmp2;
  int borrow = 0;
  for (int i = bytes - 1; i >= 0; --i)
  {
    tmp1 = (DTYPE_TMP)a[i] + (MAX_VAL + 1); /* + number_base */
    tmp2 = (DTYPE_TMP)b[i] + borrow;;
    res = (tmp1 - tmp2);
    c[i] = (DTYPE)(res & MAX_VAL); /* "modulo number_base" == "% (number_base - 1)" if number_base is 2^N */
    borrow = (res <= MAX_VAL);
  }

  bignum_truncate(c, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
}

void bignum_mul(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);

  uint8_t* row = malloc(bytes * sizeof(uint8_t));
  uint8_t* tmp = malloc(bytes * sizeof(uint8_t));

  memset(row, 0, bytes);
  memset(tmp, 0, bytes);

  for (int i = (bytes - 1); i >= 0; --i)
  {
    memset(row, 0, bytes);

    for (int j = (bytes - 1); j >= 0; --j)
    {
      if (i + j + 1 >= bytes)
      {
        memset(tmp, 0, bytes);

        uint16_t intermediate = ((uint16_t)a[i] * (uint16_t)b[j]);
        bignum_from_int_signed(tmp, intermediate, bytes);  
        _rshift_word(tmp, (bytes - 1 - i) + (bytes - 1 - j), bytes);
        bignum_add(tmp, row, row, length_in_bits);
      }
    }
    bignum_add(c, row, c, length_in_bits);
  }

  bignum_truncate(c, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
  free(tmp);
  free(row);
}

void bignum_div(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);

  if (bignum_cmp(a, b, bytes) == -1) {
    memset(c, 0, bytes);
    return;
  }

  uint8_t* current = malloc(bytes * sizeof(uint8_t));
  uint8_t* denom = malloc(bytes * sizeof(uint8_t));
  uint8_t* tmp = malloc(bytes * sizeof(uint8_t));

  bignum_from_int_signed(current, 1, bytes);               // int current = 1;
  bignum_assign(denom, b, bytes);                   // denom = b
  bignum_assign(tmp, a, bytes);                     // tmp   = a

  const DTYPE_TMP half_max = 1 + (DTYPE_TMP)(MAX_VAL / 2);
  bool overflow = false;
  while (bignum_cmp(denom, a, bytes) != LARGER)     // while (denom <= a) {
  {
    if (denom[0] >= half_max)
    {
      overflow = true;  
      break;
    }
    _lshift_one_bit(current, bytes);                //   current <<= 1;
    _lshift_one_bit(denom, bytes);                  //   denom <<= 1;
  }
  if (!overflow)
  {
    _rshift_one_bit(denom, bytes);                  // denom >>= 1;
    _rshift_one_bit(current, bytes);                // current >>= 1;
  }
  // bignum_init(c);                             // int answer = 0;

  while (!bignum_is_zero(current, bytes))           // while (current != 0)
  {
    if (bignum_cmp(tmp, denom, bytes) != SMALLER)  //   if (dividend >= denom)
    {
      bignum_sub(tmp, denom, tmp, length_in_bits);         //     dividend -= denom;
      bignum_or(c, current, c, bytes);              //     answer |= current;
    }
    _rshift_one_bit(current, bytes);                //   current >>= 1;
    _rshift_one_bit(denom, bytes);                  //   denom >>= 1;
  }                                         // return answer;

  bignum_truncate(c, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
  free(tmp);
  free(denom);
  free(current);
}

void bignum_lshift(uint8_t* a, uint8_t* b, int nbits, uint8_t length_in_bits)
{
  require(a, "a is null");
  require(b, "b is null");
  require(nbits >= 0, "no negative shifts");

  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  bignum_assign(b, a, bytes);
  /* Handle shift in multiples of word-size */
  int nwords = nbits / 8;
  if (nwords != 0)
  {
    _rshift_word(b, nwords, bytes);
    nbits -= (nwords * 8);
  }

  if (nbits != 0)
  {
    int i;
    for (i = 0; i < bytes - 1; ++i)
      b[i] = (b[i] << nbits) | (b[i + 1] >> (8 - nbits));
      
    b[i] <<= nbits;
  }

  bignum_truncate(b, first_bits);
}

void bignum_rshift(uint8_t* a, uint8_t* b, int nbits, uint8_t length_in_bits)
{
  require(a, "a is null");
  require(b, "b is null");
  require(nbits >= 0, "no negative shifts");

  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);
  
  bignum_assign(b, a, bytes);
  /* Handle shift in multiples of word-size */
  int nwords = nbits / 8;
  if (nwords != 0)
  {
    _lshift_word(b, nwords, bytes);
    nbits -= (nwords * 8);
  }

  if (nbits != 0)
  {
    for (int i = bytes - 1; i > 0; --i)
      b[i] = (b[i] >> nbits) | (b[i - 1] << (8 - nbits));

    b[0] >>= nbits;
  }

  bignum_truncate(b, first_bits);
}

void bignum_lshift_signed(uint8_t* a, uint8_t* b, int nbits, uint8_t length_in_bits) {
  bignum_lshift(a, b, nbits, length_in_bits);
}

void bignum_rshift_signed(uint8_t* a, uint8_t* b, int nbits, uint8_t length_in_bits) {  
  uint8_t bytes = BITS_TO_BYTES(length_in_bits); // 4
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits); // 4

  uint8_t sign = GET_SIGN(a, first_bits);

  if (!sign) {
    bignum_rshift(a, b, nbits, length_in_bits);
    return;
  }

  if (nbits > length_in_bits) {
    for (int i = 0; i < bytes; i++)
      b[i] = 0;

    return;
  }

  bignum_assign(b, a, bytes);

  int nwords = nbits / 8;
  if (nwords > 0) {
    _rshift_word_signed(b, nwords, length_in_bits);
    nbits -= nwords * 8;
  }

  if (nbits != 0) {
    for (int i = bytes - 1; i > 0; i--)
      b[i] = (b[i] >> nbits) | (b[i - 1] << (8 - nbits));

    b[0] >>= nbits;

    // We need to add the 1s at the beginning.
    if (first_bits >= nbits) {
      // Mask to cover the bits to be replaced by ones
      uint8_t mask = (uint8_t)BIT_MASK(first_bits) ^ (uint8_t)BIT_MASK(first_bits - nbits);
      b[0] |= mask;
    }
    else {
      uint8_t mask = BIT_MASK(first_bits);
      b[0] |= mask;

      // Even in the second byte
      if (bytes > 1) {
        uint8_t remaining = 8 - (nbits - first_bits);
        mask = 0xff ^ (uint8_t)BIT_MASK(remaining);
        b[1] |= mask;
      }
    }
  }

  bignum_truncate(b, first_bits);
}

void _rshift_word_signed(uint8_t* a, int nwords, uint8_t length_in_bits) {
  if (nwords <= 0)
    return; 

  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);
  uint8_t sign = GET_SIGN(a, first_bits);

  uint8_t first_byte_mask = BIT_MASK(first_bits);

  if (!sign) {
    _rshift_word(a, nwords, bytes);
    return;
  }

  uint8_t mask = 255 ^ first_byte_mask;

  int i = bytes - 1;
  for (; i >= nwords; i--)
    a[i] = a[i - nwords];

  a[nwords] |= mask;

  for (; i >= 1; i--)
    a[i] = 0xff;

  a[0] = first_byte_mask;
}

void bignum_mod(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length)
{
  /*
    Take divmod and throw away div part
  */
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");

  uint8_t* tmp = malloc(length * sizeof(uint8_t));

  bignum_divmod(a,b,tmp,c, length);
}

void bignum_divmod(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t* d, uint8_t length)
{
  /*
    Puts a%b in d
    and a/b in c

    mod(a,b) = a - ((a / b) * b)

    example:
      mod(8, 3) = 8 - ((8 / 3) * 3) = 2
  */
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");

  uint8_t* tmp = malloc(length * sizeof(uint8_t));

  /* c = (a / b) */
  bignum_div(a, b, c, length);

  /* tmp = (c * b) */
  bignum_mul(c, b, tmp, length);

  /* c = a - tmp */
  bignum_sub(a, tmp, d, length);
}

void bignum_and(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bytes)
{
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");

  for (int i = 0; i < length_in_bytes; ++i)
    c[i] = (a[i] & b[i]);
}

void bignum_or(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bytes)
{
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");

  for (int i = 0; i < length_in_bytes; ++i)
    c[i] = (a[i] | b[i]);
}

void bignum_xor(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bytes)
{
  require(a, "a is null");
  require(b, "b is null");
  require(c, "c is null");

  for (int i = 0; i < length_in_bytes; ++i)
    c[i] = (a[i] ^ b[i]);
}

int bignum_cmp(uint8_t* a, uint8_t* b, uint8_t length)
{
  require(a, "a is null");
  require(b, "b is null");

  int i = -1;
  do
  {
    i += 1; /* Decrement first, to start with last array element */
    if (a[i] > b[i])
      return LARGER;
      
    else if (a[i] < b[i])
      return SMALLER;
          
  }
  while (i != length - 1);

  return EQUAL;
}

int bignum_gr_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  int r = bignum_cmp_signed(a, b, length_in_bits);
  return r > 0;
}

int bignum_ge_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  int r = bignum_cmp_signed(a, b, length_in_bits);
  return r >= 0;
}

int bignum_ls_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  int r = bignum_cmp_signed(a, b, length_in_bits);
  return r < 0;
}

int bignum_le_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  int r = bignum_cmp_signed(a, b, length_in_bits);
  return r <= 0;
}

// a > b => 1;
// b > a => -1;
int bignum_cmp_signed(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  uint8_t signA = GET_SIGN(a, first_bits);
  uint8_t signB = GET_SIGN(b, first_bits);;

  if (signA == signB)
    return memcmp(a, b, bytes);

  return signB - signA;
}

int bignum_is_zero(uint8_t* n, uint8_t length)
{
  require(n, "n is null");

  for (int i = 0; i < length; ++i)
    if (n[i])
      return 0;

  return 1;
}

void bignum_negate(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);
  uint8_t sign = GET_SIGN(a, first_bits);

  if (sign)
    bignum_make_positive(a, b, length_in_bits);
  else
    bignum_make_negative(a, b, length_in_bits);
}

// void bignum_pow(struct bn* a, struct bn* b, struct bn* c)
// {
//   require(a, "a is null");
//   require(b, "b is null");
//   require(c, "c is null");

//   struct bn tmp;

//   bignum_init(c);

//   if (bignum_cmp(b, c) == EQUAL)
//   {
//     /* Return 1 when exponent is 0 -- n^0 = 1 */
//     bignum_inc(c);
//   }
//   else
//   {
//     struct bn bcopy;
//     bignum_assign(&bcopy, b);

//     /* Copy a -> tmp */
//     bignum_assign(&tmp, a);

//     bignum_dec(&bcopy);
 
//     /* Begin summing products: */
//     while (!bignum_is_zero(&bcopy))
//     {

//       /* c = tmp * tmp */
//       bignum_mul(&tmp, a, c);
//       /* Decrement b by one */
//       bignum_dec(&bcopy);

//       bignum_assign(&tmp, c);
//     }

//     /* c = tmp */
//     bignum_assign(c, &tmp);
//   }
// }

// void bignum_isqrt(struct bn *a, struct bn* b)
// {
//   require(a, "a is null");
//   require(b, "b is null");

//   struct bn low, high, mid, tmp;

//   bignum_init(&low);
//   bignum_assign(&high, a);
//   bignum_rshift(&high, &mid, 1);
//   bignum_inc(&mid);

//   while (bignum_cmp(&high, &low) > 0) 
//   {
//     bignum_mul(&mid, &mid, &tmp);
//     if (bignum_cmp(&tmp, a) > 0) 
//     {
//       bignum_assign(&high, &mid);
//       bignum_dec(&high);
//     }
//     else 
//     {
//       bignum_assign(&low, &mid);
//     }
//     bignum_sub(&high,&low,&mid);
//     _rshift_one_bit(&mid);
//     bignum_add(&low,&mid,&mid);
//     bignum_inc(&mid);
//   }
//   bignum_assign(b,&low);
// }

void bignum_assign(uint8_t* dst, uint8_t* src, uint8_t length_in_bytes)
{
  require(dst, "dst is null");
  require(src, "src is null");

  for (int i = 0; i < length_in_bytes; ++i)
    dst[i] = src[i];
}

void bignum_concat_arr_arr(uint8_t* a, uint8_t length_a_in_bits, uint8_t* b, uint8_t length_b_in_bits, uint8_t* c) {
  uint8_t length_a_in_bytes = BITS_TO_BYTES(length_a_in_bits); // 1
  uint8_t length_b_in_bytes = BITS_TO_BYTES(length_b_in_bits); // 1
  uint8_t first_bits_a = BITS_IN_FIRST_BYTE(length_a_in_bytes, length_a_in_bits); // 3
  uint8_t first_bits_b = BITS_IN_FIRST_BYTE(length_b_in_bytes, length_b_in_bits); // 3

  uint8_t new_length_in_bits = length_a_in_bits + length_b_in_bits; // 6
  uint8_t new_length_in_bytes = BITS_TO_BYTES(new_length_in_bits); // 1

  uint8_t *tmp_a = malloc(length_a_in_bytes * sizeof(uint8_t));
  uint8_t *tmp_b = malloc(length_b_in_bytes * sizeof(uint8_t));
  bignum_assign(tmp_b, b, length_b_in_bytes);

  uint8_t mask = BIT_MASK(length_b_in_bytes - length_b_in_bits);
  uint8_t value_to_be_moved = (mask & a[length_a_in_bytes - 1]) << first_bits_b;

  tmp_b[0] |= value_to_be_moved;

  bignum_rshift(a, tmp_a, length_b_in_bytes - length_b_in_bits, length_a_in_bits); // a, tmp_a, 5, 3

  for (int i = 0; i < new_length_in_bytes - length_b_in_bytes; i++) 
    c[i] = tmp_a[length_a_in_bytes - (new_length_in_bytes - length_a_in_bytes) + i];

  for (int i = 0; i < length_b_in_bytes; i++)
    c[new_length_in_bytes - length_b_in_bytes + i] = tmp_b[i];

  free(tmp_b);
  free(tmp_a);
}

void bignum_concat_arr_int(uint8_t* a, uint8_t length_a_in_bits, uint32_t b, uint8_t length_b_in_bits, uint8_t* c) {
  uint8_t length_b_in_bytes = BITS_TO_BYTES(length_b_in_bits);
  uint8_t *tmp = malloc(length_b_in_bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, b, length_b_in_bytes);

  bignum_concat_arr_arr(a, length_a_in_bits, tmp, length_b_in_bits, c);
  free(tmp);
}

void bignum_concat_int_int(uint32_t a, uint8_t length_a_in_bits, uint32_t b, uint8_t length_b_in_bits, uint8_t* c) {
  uint8_t length_a_in_bytes = BITS_TO_BYTES(length_a_in_bits);
  uint8_t length_b_in_bytes = BITS_TO_BYTES(length_b_in_bits);
  uint8_t *tmp_a = malloc(length_a_in_bytes * sizeof(uint8_t));
  uint8_t *tmp_b = malloc(length_b_in_bytes * sizeof(uint8_t));

  bignum_from_int_signed(tmp_a, a, length_a_in_bytes);
  bignum_from_int_signed(tmp_b, b, length_b_in_bytes);

  bignum_concat_arr_arr(tmp_a, length_a_in_bits, tmp_b, length_b_in_bits, c);
  free(tmp_b);
  free(tmp_a);
}

void bignum_concat_int_arr(uint32_t a, uint8_t length_a_in_bits, uint8_t* b, uint8_t length_b_in_bits, uint8_t* c) {
  uint8_t length_a_in_bytes = BITS_TO_BYTES(length_a_in_bits);
  uint8_t *tmp = malloc(length_a_in_bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, a, length_a_in_bytes);

  bignum_concat_arr_arr(tmp, length_a_in_bits, b, length_a_in_bits, c);
  free(tmp);
}

void bignum_slice(uint8_t* a, uint8_t length, uint8_t* c, uint8_t from, uint8_t to) {
  uint8_t from_byte = from / 8;
  uint8_t to_byte = to / 8 + 1;
  uint8_t new_length = to_byte - from_byte;

  for (int i = 0; i < new_length; i++)
    c[i] = a[from_byte + i];

  uint8_t r_shift_value = to_byte * 8 - to - 1;

  bignum_rshift(c, c, r_shift_value, new_length);

  uint8_t rest = (to_byte * 8 - to - 1) + (from - from_byte * 8);

  uint8_t mask = BIT_MASK(8 - rest);
  c[0] &= mask;
}

uint32_t bignum_slice_int(uint8_t* a, uint8_t length, uint8_t from, uint8_t to) {
  uint8_t from_byte = from / 8; // 0
  uint8_t to_byte = to / 8 + 1; // 2
  uint8_t new_length = to_byte - from_byte; // 2

  uint8_t *tmp = malloc(new_length * sizeof(uint8_t));
  bignum_slice(a, length, tmp, from, to);

  int r = bignum_to_int(tmp, new_length);

  free(tmp);
  return r;
}

void bignum_truncate(uint8_t* a, uint8_t remaining_bits) {
  uint8_t mask = BIT_MASK(remaining_bits);
  a[0] &= mask;
}

// /* Private / Static functions. */
void _rshift_word(uint8_t* a, int nwords, uint8_t length)
{
  /* Naive method: */
  require(a, "a is null");
  require(nwords >= 0, "no negative shifts");

  int i;
  if (nwords >= length)
  {
    for (i = 0; i < length; ++i)
      a[i] = 0;
      
    return;
  }

  for (i = 0; i < length - nwords; ++i)
    a[i] = a[i + nwords];
    
  for (; i < length; ++i)
    a[i] = 0;
}

void _lshift_word(uint8_t* a, int nwords, uint8_t length)
{
  require(a, "a is null");
  require(nwords >= 0, "no negative shifts");

  int i;
  /* Shift whole words */
  for (i = (length - 1); i >= nwords; --i)
  {
    a[i] = a[i - nwords];
  }
  /* Zero pad shifted words. */
  for (; i >= 0; --i)
  {
    a[i] = 0;
  }  
}

void _lshift_one_bit(uint8_t* a, uint8_t length)
{
  require(a, "a is null");

  for (int i = 0; i < length - 1; ++i)
    a[i] = (a[i] << 1) | (a[i + 1] >> 7);
    
  a[length - 1] <<= 1;
}

void _rshift_one_bit(uint8_t* a, uint8_t length)
{
  require(a, "a is null");

  for (int i = length - 1; i >= 1; --i)
    a[i] = (a[i] >> 1) | (a[i - 1] << 7);
    
  a[0] >>= 1;
}
