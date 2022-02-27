#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "bn.h"
#include <math.h>

#define MIN(a,b) (a < b ? a : b)

void _lshift_word(uint8_t* a, int nwords, uint8_t length);
void _rshift_word(uint8_t* a, int nwords, uint8_t length);

void bignum_make_positive(uint8_t* a, uint8_t* b, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t* tmp = malloc(bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, 1, bytes);

  bignum_sub(a, tmp, b, length_in_bits);
  bignum_not(b, b, length_in_bits);

  bignum_truncate(b, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
  free(tmp);
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
  uint8_t length_in_bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t bits_in_first = BITS_IN_FIRST_BYTE(length_in_bytes, length_in_bits);

  for (int i = 0; i < length_in_bytes; i++) 
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

void bignum_from_int_unsigned(uint8_t* n, UNSIGNED_TYPE i, uint8_t length_in_bytes) {
  int max_length = MIN(length_in_bytes, sizeof(UNSIGNED_TYPE)); // 4

  for (int j = length_in_bytes - 1; j >= length_in_bytes - max_length; j--) {
    uint32_t r = (length_in_bytes - 1 - j); // 3
    UNSIGNED_TYPE mask = ((UNSIGNED_TYPE)0xff) << (r * 8);
    n[j] = (i & mask) >> r * 8;
  }
}

void bignum_from_int_signed(uint8_t* n, SIGNED_TYPE i, uint8_t length_in_bytes)
{
  int max_length = MIN(length_in_bytes, sizeof(SIGNED_TYPE));

  for (int j = length_in_bytes - 1; j >= length_in_bytes - max_length; j--) {
    int r = (length_in_bytes - 1 - j);
    UNSIGNED_TYPE mask = ((UNSIGNED_TYPE)0xff) << (r * 8);
    n[j] = (i & mask) >> r * 8;
  }
}

UNSIGNED_TYPE bignum_to_int(uint8_t* n, uint8_t length) {
  uint8_t a = MIN(length, sizeof(UNSIGNED_TYPE));

  UNSIGNED_TYPE r = 0;
  for (int i = a - 1; i >= 0; i--) {
    uint32_t factor = (a - i - 1) * 8;
    r += (n[i] * (1 << factor));
  }

  return r;
}

SIGNED_TYPE bignum_to_int_signed(uint8_t* n, uint8_t length) {
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

void bignum_add(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t bits_in_first_byte = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  UNSIGNED_TYPE tmp;
  int carry = 0;
  for (int i = 0; i < bytes; ++i)
  {
    tmp = (UNSIGNED_TYPE)a[bytes - 1 - i] + b[bytes - 1 - i] + carry;
    carry = (tmp > 0xff);
    c[bytes - 1 - i] = (tmp & 0xff);
  }

  bignum_truncate(c, bits_in_first_byte);
}

void bignum_sub(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);

  UNSIGNED_TYPE res, tmp1, tmp2;
  int borrow = 0;
  for (int i = bytes - 1; i >= 0; --i)
  {
    tmp1 = (UNSIGNED_TYPE)a[i] + (0xff + 1); /* + number_base */
    tmp2 = (UNSIGNED_TYPE)b[i] + borrow;;
    res = (tmp1 - tmp2);
    c[i] = (uint8_t)(res & 0xff); /* "modulo number_base" == "% (number_base - 1)" if number_base is 2^N */
    borrow = (res <= 0xff);
  }

  bignum_truncate(c, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
}

void bignum_mul(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
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

void bignum_lshift(uint8_t* a, uint8_t* b, int nbits, uint8_t length_in_bits)
{
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
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

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

  uint8_t mask = 0xff ^ first_byte_mask;

  int i = bytes - 1;
  for (; i >= nwords; i--)
    a[i] = a[i - nwords];

  a[nwords] |= mask;

  for (; i >= 1; i--)
    a[i] = 0xff;

  a[0] = first_byte_mask;
}

void bignum_and(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  uint8_t length_in_bytes = BITS_TO_BYTES(length_in_bits);

  for (int i = 0; i < length_in_bytes; ++i)
    c[i] = a[i] & b[i];
}

void bignum_or(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  uint8_t length_in_bytes = BITS_TO_BYTES(length_in_bits);

  for (int i = 0; i < length_in_bytes; ++i)
    c[i] = a[i] | b[i];
}

void bignum_xor(uint8_t* a, uint8_t* b, uint8_t* c, uint8_t length_in_bits)
{
  uint8_t length_in_bytes = BITS_TO_BYTES(length_in_bits);

  for (int i = 0; i < length_in_bytes; ++i)
    c[i] = a[i] ^ b[i];
}

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

void bignum_assign(uint8_t* dst, uint8_t* src, uint8_t length_in_bytes)
{
  for (int i = 0; i < length_in_bytes; ++i)
    dst[i] = src[i];
}

void bignum_concat_arr_arr(uint8_t* a, uint8_t length_a_in_bits, uint8_t* b, uint8_t length_b_in_bits, uint8_t* c) {
  uint8_t length_a_in_bytes = BITS_TO_BYTES(length_a_in_bits);
  uint8_t length_b_in_bytes = BITS_TO_BYTES(length_b_in_bits);
  uint8_t first_bits_a = BITS_IN_FIRST_BYTE(length_a_in_bytes, length_a_in_bits);
  uint8_t first_bits_b = BITS_IN_FIRST_BYTE(length_b_in_bytes, length_b_in_bits);

  uint8_t new_length_in_bits = length_a_in_bits + length_b_in_bits;
  uint8_t new_length_in_bytes = BITS_TO_BYTES(new_length_in_bits);

  uint8_t *tmp_a = malloc(length_a_in_bytes * sizeof(uint8_t));
  uint8_t *tmp_b = malloc(length_b_in_bytes * sizeof(uint8_t));
  bignum_assign(tmp_b, b, length_b_in_bytes);

  uint8_t mask = BIT_MASK(length_b_in_bytes * 8 - length_b_in_bits);
  uint8_t value_to_be_moved = (mask & a[length_a_in_bytes - 1]) << (first_bits_b == 8 ? 0 : first_bits_b);

  tmp_b[0] |= value_to_be_moved;

  bignum_rshift(a, tmp_a, length_b_in_bytes * 8 - length_b_in_bits, length_a_in_bits);

  for (int i = 0; i < new_length_in_bytes - length_b_in_bytes; i++) 
    c[i] = tmp_a[length_a_in_bytes - (new_length_in_bytes - length_b_in_bytes) + i];

  for (int i = 0; i < length_b_in_bytes; i++)
    c[new_length_in_bytes - length_b_in_bytes + i] = tmp_b[i];

  free(tmp_b);
  free(tmp_a);
}

void bignum_concat_arr_int(uint8_t* a, uint8_t length_a_in_bits, UNSIGNED_TYPE b, uint8_t length_b_in_bits, uint8_t* c) {
  uint8_t length_b_in_bytes = BITS_TO_BYTES(length_b_in_bits);
  uint8_t *tmp = malloc(length_b_in_bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, b, length_b_in_bytes);

  bignum_concat_arr_arr(a, length_a_in_bits, tmp, length_b_in_bits, c);
  free(tmp);
}

void bignum_concat_int_int(UNSIGNED_TYPE a, uint8_t length_a_in_bits, UNSIGNED_TYPE b, uint8_t length_b_in_bits, uint8_t* c) {
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

void bignum_concat_int_arr(UNSIGNED_TYPE a, uint8_t length_a_in_bits, uint8_t* b, uint8_t length_b_in_bits, uint8_t* c) {
  uint8_t length_a_in_bytes = BITS_TO_BYTES(length_a_in_bits);
  uint8_t *tmp = malloc(length_a_in_bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, a, length_a_in_bytes);

  bignum_concat_arr_arr(tmp, length_a_in_bits, b, length_a_in_bits, c);
  free(tmp);
}

void bignum_slice(uint8_t* a, uint8_t length, uint8_t* c, uint8_t from, uint8_t to) {
  uint8_t from_byte = from / 8;
  uint8_t to_byte = BITS_TO_BYTES(to);
  uint8_t new_length = to_byte - from_byte;

  for (int i = 0; i < new_length; i++)
    c[i] = a[from_byte + i];

  uint8_t r_shift_value = to_byte * 8 - to - 1;
  bignum_rshift(c, c, r_shift_value, to - from + 1);
  uint8_t rest = (to_byte * 8 - to - 1) + (from - from_byte * 8);

  if (rest != 0) {
    uint8_t mask = BIT_MASK(8 - rest);
    c[0] &= mask;
  }
}

UNSIGNED_TYPE bignum_slice_int(uint8_t* a, uint8_t length, uint8_t from, uint8_t to) {
  uint8_t from_byte = from / 8;
  uint8_t to_byte = to / 8 + 1;
  uint8_t new_length = to_byte - from_byte;

  uint8_t *tmp = malloc(new_length * sizeof(uint8_t));
  bignum_slice(a, length, tmp, from, to);

  UNSIGNED_TYPE r = bignum_to_int(tmp, new_length);

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
  for (int i = 0; i < length - 1; ++i)
    a[i] = (a[i] << 1) | (a[i + 1] >> 7);
    
  a[length - 1] <<= 1;
}

void _rshift_one_bit(uint8_t* a, uint8_t length)
{
  for (int i = length - 1; i >= 1; --i)
    a[i] = (a[i] >> 1) | (a[i - 1] << 7);
    
  a[0] >>= 1;
}
