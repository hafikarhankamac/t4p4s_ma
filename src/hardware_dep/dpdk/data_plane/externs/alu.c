#include "alu.h"

void modulo_32(uint32_t *ret, uint32_t *div, uint32_t mod, SHORT_STDPARAMS)
{
    *ret = *div % mod;
}
void modulo_64(uint64_t *ret, uint64_t *div, uint64_t mod, SHORT_STDPARAMS)
{
    *ret = *div % mod;
}

