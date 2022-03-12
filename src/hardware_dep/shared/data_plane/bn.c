#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "bn.h"
#include <math.h>

#define MIN(a,b) (a < b ? a : b)

void _lshift_word(uint8_t* a, int nwords, uint8_t length_in_bytes);
void _rshift_word(uint8_t* a, int nwords, uint8_t length_in_bytes);

void bignum_make_positive(uint8_t* src, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t* tmp = malloc(bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, 1, bytes);

  bignum_sub(src, tmp, dst, length_in_bits);
  bignum_not(dst, dst, length_in_bits);

  bignum_truncate(dst, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
  free(tmp);
}

void bignum_make_negative(uint8_t* src, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t* tmp = malloc(bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, 1, bytes);

  bignum_not(src, dst, length_in_bits);
  bignum_add(dst, tmp, dst, length_in_bits);

  bignum_truncate(dst, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
  free(tmp);
}

void bignum_not(uint8_t* src, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t length_in_bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t bits_in_first = BITS_IN_FIRST_BYTE(length_in_bytes, length_in_bits);

  for (int i = 0; i < length_in_bytes; i++) 
    dst[i] = 0xff - src[i];

  bignum_truncate(dst, bits_in_first);
}

void bignum_add_signed(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  bignum_add(left, right, dst, length_in_bits);
}

void bignum_sub_signed(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);
  uint8_t sign = GET_SIGN(right, first_bits);

  if (sign) {
    uint8_t* tmp = malloc(bytes * sizeof(uint8_t));
    bignum_make_positive(right, tmp, length_in_bits);
    bignum_add(left, tmp, dst, length_in_bits);
    free(tmp);
  }
  else {
    bignum_sub(left, right, dst, length_in_bits);
  }
}

void bignum_mul_signed(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  uint8_t is_left_neg = GET_SIGN(left, first_bits);
  uint8_t is_right_neg = GET_SIGN(right, first_bits);;

  uint8_t *tmp1, *tmp2;

  if (is_left_neg) {
    tmp1 = malloc(bytes * sizeof(uint8_t));
    bignum_make_positive(left, tmp1, length_in_bits);
  }
  else
    tmp1 = left;

  if (is_right_neg) {
    tmp2 = malloc(bytes * sizeof(uint8_t));
    bignum_make_positive(right, tmp2, length_in_bits);
  }
  else
    tmp2 = right;

  bignum_mul(tmp1, tmp2, dst, length_in_bits);

  if (is_left_neg != is_right_neg)
    bignum_make_negative(dst, dst, length_in_bits);

  if (is_right_neg)
    free(tmp2);

  if (is_left_neg)
    free(tmp1);
}

void bignum_from_int_unsigned(uint8_t* dst, UNSIGNED_TYPE src, uint8_t length_in_bytes) {
  int max_length = MIN(length_in_bytes, sizeof(UNSIGNED_TYPE)); // 4

  for (int j = length_in_bytes - 1; j >= length_in_bytes - max_length; j--) {
    uint32_t r = (length_in_bytes - 1 - j); // 3
    UNSIGNED_TYPE mask = ((UNSIGNED_TYPE)0xff) << (r * 8);
    dst[j] = (src & mask) >> r * 8;
  }
}

void bignum_from_int_signed(uint8_t* dst, SIGNED_TYPE src, uint8_t length_in_bytes) {
  int max_length = MIN(length_in_bytes, sizeof(SIGNED_TYPE));

  for (int j = length_in_bytes - 1; j >= length_in_bytes - max_length; j--) {
    int r = (length_in_bytes - 1 - j);
    UNSIGNED_TYPE mask = ((UNSIGNED_TYPE)0xff) << (r * 8);
    dst[j] = (src & mask) >> r * 8;
  }
}

UNSIGNED_TYPE bignum_to_int(uint8_t* src, uint8_t length_in_bytes) {
  uint8_t min_length = MIN(length_in_bytes, sizeof(UNSIGNED_TYPE));

  UNSIGNED_TYPE r = 0;
  for (int i = min_length - 1; i >= 0; i--) {
    uint32_t factor = (min_length - i - 1) * 8;
    r += (src[i] * (1 << factor));
  }

  return r;
}

SIGNED_TYPE bignum_to_int_signed(uint8_t* src, uint8_t length_in_bytes) {
  if (!(src[0] >> 7))
    return bignum_to_int(src, length_in_bytes);

  uint8_t *temp = malloc(length_in_bytes * sizeof(uint8_t));
  uint8_t *temp2 = malloc(length_in_bytes * sizeof(uint8_t));
  temp2[length_in_bytes - 1] = 1;

  bignum_sub(src, temp2, temp, length_in_bytes*8);
  bignum_not(temp, temp, length_in_bytes*8);

  int as_int = bignum_to_int(temp, length_in_bytes);

  free(temp2);
  free(temp);
  return -as_int;
}

void bignum_cast(uint8_t* src, uint8_t length_src_in_bits, uint8_t* dst, uint8_t length_dst_in_bits) {
  int length_left_in_bytes = BITS_TO_BYTES(length_src_in_bits);
  int length_right_in_bytes = BITS_TO_BYTES(length_dst_in_bits);

  uint8_t bits_in_first_right_byte = BITS_IN_FIRST_BYTE(length_right_in_bytes, length_dst_in_bits);
  uint8_t mask = BIT_MASK(bits_in_first_right_byte);

  int min_length = MIN(length_left_in_bytes, length_right_in_bytes);

  for (int i = 0; i < min_length; i++)
    dst[length_right_in_bytes - i - 1] = src[length_left_in_bytes - i - 1];

  dst[length_right_in_bytes - min_length] &= mask;

  for (int i = 0; i < length_right_in_bytes - min_length; i++)
    dst[i] = 0;
}

void bignum_cast_signed(uint8_t* src, uint8_t length_src_in_bits, uint8_t* dst, uint8_t length_dst_in_bits) {
  int length_left_in_bytes = BITS_TO_BYTES(length_src_in_bits);
  int length_right_in_bytes = BITS_TO_BYTES(length_dst_in_bits);

  uint8_t bits_in_first_b_byte = BITS_IN_FIRST_BYTE(length_right_in_bytes, length_dst_in_bits);
  uint8_t bits_in_first_a_byte = BITS_IN_FIRST_BYTE(length_left_in_bytes, length_src_in_bits);

  uint8_t sign = GET_SIGN(src, bits_in_first_a_byte);

  uint8_t min_length = MIN(length_left_in_bytes, length_right_in_bytes);

  for (int i = 0; i < min_length; i++)
    dst[length_right_in_bytes - i - 1] = src[length_left_in_bytes - i - 1];

  if (length_dst_in_bits > length_src_in_bits && sign) {
    uint8_t x = length_right_in_bytes > length_left_in_bytes ? 8 : bits_in_first_b_byte;
    uint8_t mask = BIT_MASK(x) ^ BIT_MASK(bits_in_first_a_byte);
    dst[length_right_in_bytes - min_length] |= mask;
  }
  else {
    dst[length_right_in_bytes - min_length] &= BIT_MASK(bits_in_first_b_byte);
  }

  for (int i = 0; i < length_right_in_bytes - min_length; i++)
    dst[i] = sign ? 0xff : 0;
}

void bignum_add(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t bits_in_first_byte = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  UNSIGNED_TYPE tmp;
  int carry = 0;
  for (int i = 0; i < bytes; ++i) {
    tmp = (UNSIGNED_TYPE)left[bytes - 1 - i] + right[bytes - 1 - i] + carry;
    carry = (tmp > 0xff);
    dst[bytes - 1 - i] = (tmp & 0xff);
  }

  bignum_truncate(dst, bits_in_first_byte);
}

void bignum_sub(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);

  UNSIGNED_TYPE res, tmp1, tmp2;
  int borrow = 0;
  for (int i = bytes - 1; i >= 0; --i) {
    tmp1 = (UNSIGNED_TYPE)left[i] + (0xff + 1);
    tmp2 = (UNSIGNED_TYPE)right[i] + borrow;;
    res = (tmp1 - tmp2);
    dst[i] = (uint8_t)(res & 0xff);
    borrow = (res <= 0xff);
  }

  bignum_truncate(dst, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
}

void bignum_mul(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);

  uint8_t* row = malloc(bytes * sizeof(uint8_t));
  uint8_t* tmp = malloc(bytes * sizeof(uint8_t));

  memset(row, 0, bytes);
  memset(tmp, 0, bytes);

  for (int i = (bytes - 1); i >= 0; --i) {
    memset(row, 0, bytes);

    for (int j = (bytes - 1); j >= 0; --j) {
      if (i + j + 1 >= bytes) {
        memset(tmp, 0, bytes);
        uint16_t intermediate = ((uint16_t)left[i] * (uint16_t)right[j]);
        bignum_from_int_signed(tmp, intermediate, bytes);  
        _rshift_word(tmp, (bytes - 1 - i) + (bytes - 1 - j), bytes);
        bignum_add(tmp, row, row, length_in_bits);
      }
    }
    bignum_add(dst, row, dst, length_in_bits);
  }

  bignum_truncate(dst, BITS_IN_FIRST_BYTE(bytes, length_in_bits));
  free(tmp);
  free(row);
}

void bignum_lshift(uint8_t* src, uint8_t* dst, int nbits, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  bignum_assign(dst, src, bytes);
  int nwords = nbits / 8;
  if (nwords != 0) {
    _rshift_word(dst, nwords, bytes);
    nbits -= (nwords * 8);
  }

  if (nbits != 0) {
    int i;
    for (i = 0; i < bytes - 1; ++i)
      dst[i] = (dst[i] << nbits) | (dst[i + 1] >> (8 - nbits));
      
    dst[i] <<= nbits;
  }

  bignum_truncate(dst, first_bits);
}

void bignum_rshift(uint8_t* src, uint8_t* dst, int nbits, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);
  
  bignum_assign(dst, src, bytes);
  int nwords = nbits / 8;
  if (nwords != 0) {
    _lshift_word(dst, nwords, bytes);
    nbits -= (nwords * 8);
  }

  if (nbits != 0) {
    for (int i = bytes - 1; i > 0; --i)
      dst[i] = (dst[i] >> nbits) | (dst[i - 1] << (8 - nbits));

    dst[0] >>= nbits;
  }

  bignum_truncate(dst, first_bits);
}

void bignum_lshift_signed(uint8_t* src, uint8_t* dst, int nbits, uint8_t length_in_bits) {
  bignum_lshift(src, dst, nbits, length_in_bits);
}

void bignum_rshift_signed(uint8_t* src, uint8_t* dst, int nbits, uint8_t length_in_bits) {  
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  uint8_t sign = GET_SIGN(src, first_bits);

  if (!sign) {
    bignum_rshift(src, dst, nbits, length_in_bits);
    return;
  }

  if (nbits > length_in_bits) {
    for (int i = 0; i < bytes; i++)
      dst[i] = 0;

    return;
  }

  bignum_assign(dst, src, bytes);

  int nwords = nbits / 8;
  if (nwords > 0) {
    _rshift_word_signed(dst, nwords, length_in_bits);
    nbits -= nwords * 8;
  }

  if (nbits != 0) {
    for (int i = bytes - 1; i > 0; i--)
      dst[i] = (dst[i] >> nbits) | (dst[i - 1] << (8 - nbits));

    dst[0] >>= nbits;

    // We need to add the 1s at the beginning.
    if (first_bits >= nbits) {
      // Mask to cover the bits to be replaced by ones
      uint8_t mask = (uint8_t)BIT_MASK(first_bits) ^ (uint8_t)BIT_MASK(first_bits - nbits);
      dst[0] |= mask;
    }
    else {
      uint8_t mask = BIT_MASK(first_bits);
      dst[0] |= mask;

      // Even in the second byte
      if (bytes > 1) {
        uint8_t remaining = 8 - (nbits - first_bits);
        mask = 0xff ^ (uint8_t)BIT_MASK(remaining);
        dst[1] |= mask;
      }
    }
  }

  bignum_truncate(dst, first_bits);
}

void _rshift_word_signed(uint8_t* target, int nwords, uint8_t length_in_bits) {
  if (nwords <= 0)
    return; 

  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);
  uint8_t sign = GET_SIGN(target, first_bits);

  uint8_t first_byte_mask = BIT_MASK(first_bits);

  if (!sign) {
    _rshift_word(target, nwords, bytes);
    return;
  }

  uint8_t mask = 0xff ^ first_byte_mask;

  int i = bytes - 1;
  for (; i >= nwords; i--)
    target[i] = target[i - nwords];

  target[nwords] |= mask;

  for (; i >= 1; i--)
    target[i] = 0xff;

  target[0] = first_byte_mask;
}

void bignum_and(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t length_in_bytes = BITS_TO_BYTES(length_in_bits);

  for (int i = 0; i < length_in_bytes; ++i)
    dst[i] = left[i] & right[i];
}

void bignum_or(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t length_in_bytes = BITS_TO_BYTES(length_in_bits);

  for (int i = 0; i < length_in_bytes; ++i)
    dst[i] = left[i] | right[i];
}

void bignum_xor(uint8_t* left, uint8_t* right, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t length_in_bytes = BITS_TO_BYTES(length_in_bits);

  for (int i = 0; i < length_in_bytes; ++i)
    dst[i] = left[i] ^ right[i];
}

int bignum_cmp_signed(uint8_t* left, uint8_t* right, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);

  uint8_t signA = GET_SIGN(left, first_bits);
  uint8_t signB = GET_SIGN(right, first_bits);;

  if (signA == signB)
    return memcmp(left, right, bytes);

  return signB - signA;
}

void bignum_negate(uint8_t* src, uint8_t* dst, uint8_t length_in_bits) {
  uint8_t bytes = BITS_TO_BYTES(length_in_bits);
  uint8_t first_bits = BITS_IN_FIRST_BYTE(bytes, length_in_bits);
  uint8_t sign = GET_SIGN(src, first_bits);

  if (sign)
    bignum_make_positive(src, dst, length_in_bits);
  else
    bignum_make_negative(src, dst, length_in_bits);
}

void bignum_assign(uint8_t* dst, uint8_t* src, uint8_t length_in_bytes) {
  for (int i = 0; i < length_in_bytes; ++i)
    dst[i] = src[i];
}

void bignum_concat_arr_arr(uint8_t* left, uint8_t legnth_left_in_bits, uint8_t* right, uint8_t length_right_in_bits, uint8_t* dst) {
  uint8_t length_left_in_bytes = BITS_TO_BYTES(legnth_left_in_bits);
  uint8_t length_right_in_bytes = BITS_TO_BYTES(length_right_in_bits);
  uint8_t first_bits_left = BITS_IN_FIRST_BYTE(length_left_in_bytes, legnth_left_in_bits);
  uint8_t first_bits_right = BITS_IN_FIRST_BYTE(length_right_in_bytes, length_right_in_bits);

  uint8_t new_length_in_bits = legnth_left_in_bits + length_right_in_bits;
  uint8_t new_length_in_bytes = BITS_TO_BYTES(new_length_in_bits);

  uint8_t *tmp_left = malloc(length_left_in_bytes * sizeof(uint8_t));
  uint8_t *tmp_right = malloc(length_right_in_bytes * sizeof(uint8_t));
  bignum_assign(tmp_right, right, length_right_in_bytes);

  uint8_t mask = BIT_MASK(length_right_in_bytes * 8 - length_right_in_bits);
  uint8_t value_to_be_moved = (mask & left[length_left_in_bytes - 1]) << (first_bits_right == 8 ? 0 : first_bits_right);

  tmp_right[0] |= value_to_be_moved;

  bignum_rshift(left, tmp_left, length_right_in_bytes * 8 - length_right_in_bits, legnth_left_in_bits);

  for (int i = 0; i < new_length_in_bytes - length_right_in_bytes; i++) 
    dst[i] = tmp_left[length_left_in_bytes - (new_length_in_bytes - length_right_in_bytes) + i];

  for (int i = 0; i < length_right_in_bytes; i++)
    dst[new_length_in_bytes - length_right_in_bytes + i] = tmp_right[i];

  free(tmp_right);
  free(tmp_left);
}

void bignum_concat_arr_int(uint8_t* left, uint8_t legnth_left_in_bits, UNSIGNED_TYPE right, uint8_t length_right_in_bits, uint8_t* dst) {
  uint8_t length_right_in_bytes = BITS_TO_BYTES(length_right_in_bits);
  uint8_t *tmp = malloc(length_right_in_bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, right, length_right_in_bytes);

  bignum_concat_arr_arr(left, legnth_left_in_bits, tmp, length_right_in_bits, dst);
  free(tmp);
}

void bignum_concat_int_int(UNSIGNED_TYPE left, uint8_t legnth_left_in_bits, UNSIGNED_TYPE right, uint8_t length_right_in_bits, uint8_t* dst) {
  uint8_t length_left_in_bytes = BITS_TO_BYTES(legnth_left_in_bits);
  uint8_t length_right_in_bytes = BITS_TO_BYTES(length_right_in_bits);
  uint8_t *tmp_left = malloc(length_left_in_bytes * sizeof(uint8_t));
  uint8_t *tmp_right = malloc(length_right_in_bytes * sizeof(uint8_t));

  bignum_from_int_signed(tmp_left, left, length_left_in_bytes);
  bignum_from_int_signed(tmp_right, right, length_right_in_bytes);

  bignum_concat_arr_arr(tmp_left, legnth_left_in_bits, tmp_right, length_right_in_bits, dst);
  free(tmp_right);
  free(tmp_left);
}

void bignum_concat_int_arr(UNSIGNED_TYPE left, uint8_t legnth_left_in_bits, uint8_t* right, uint8_t length_right_in_bits, uint8_t* dst) {
  uint8_t length_left_in_bytes = BITS_TO_BYTES(legnth_left_in_bits);
  uint8_t *tmp = malloc(length_left_in_bytes * sizeof(uint8_t));
  bignum_from_int_signed(tmp, left, length_left_in_bytes);

  bignum_concat_arr_arr(tmp, legnth_left_in_bits, right, length_right_in_bits, dst);
  free(tmp);
}

void bignum_slice(uint8_t* src, uint8_t length, uint8_t* dst, uint8_t from, uint8_t to) {
  uint8_t from_byte = from / 8;
  uint8_t to_byte = BITS_TO_BYTES(to);
  uint8_t new_length = to_byte - from_byte;

  for (int i = 0; i < new_length; i++)
    dst[i] = src[from_byte + i];

  uint8_t r_shift_value = to_byte * 8 - to - 1;
  bignum_rshift(dst, dst, r_shift_value, to - from + 1);
  uint8_t rest = (to_byte * 8 - to - 1) + (from - from_byte * 8);

  if (rest != 0) {
    uint8_t mask = BIT_MASK(8 - rest);
    dst[0] &= mask;
  }
}

UNSIGNED_TYPE bignum_slice_int(uint8_t* src, uint8_t length, uint8_t from, uint8_t to) {
  uint8_t from_byte = from / 8;
  uint8_t to_byte = to / 8 + 1;
  uint8_t new_length = to_byte - from_byte;

  uint8_t *tmp = malloc(new_length * sizeof(uint8_t));
  bignum_slice(src, length, tmp, from, to);

  UNSIGNED_TYPE r = bignum_to_int(tmp, new_length);

  free(tmp);
  return r;
}

void bignum_truncate(uint8_t* target, uint8_t remaining_bits) {
  uint8_t mask = BIT_MASK(remaining_bits);
  target[0] &= mask;
}

// /* Private / Static functions. */
void _rshift_word(uint8_t* target, int nwords, uint8_t length_in_bytes) {
  int i;
  if (nwords >= length_in_bytes) {
    for (i = 0; i < length_in_bytes; ++i)
      target[i] = 0;
      
    return;
  }

  for (i = 0; i < length_in_bytes - nwords; ++i)
    target[i] = target[i + nwords];
    
  for (; i < length_in_bytes; ++i)
    target[i] = 0;
}

void _lshift_word(uint8_t* target, int nwords, uint8_t length_in_bytes) {
  int i;
  /* Shift whole words */
  for (i = (length_in_bytes - 1); i >= nwords; --i)
    target[i] = target[i - nwords];
    
  /* Zero pad shifted words. */
  for (; i >= 0; --i)
    target[i] = 0;
}

void _lshift_one_bit(uint8_t* target, uint8_t length_in_bytes) {
  for (int i = 0; i < length_in_bytes - 1; ++i)
    target[i] = (target[i] << 1) | (target[i + 1] >> 7);
    
  target[length_in_bytes - 1] <<= 1;
}

void _rshift_one_bit(uint8_t* target, uint8_t length_in_bytes) {
  for (int i = length_in_bytes - 1; i >= 1; --i)
    target[i] = (target[i] >> 1) | (target[i - 1] << 7);
    
  target[0] >>= 1;
}
