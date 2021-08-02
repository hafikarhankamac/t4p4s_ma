#include "alu.h"

void modulo_8(uint32_t declargs, uint8_t *ret, uint8_t div, uint8_t mod, alu_t* alu, SHORT_STDPARAMS) {
    *ret = div % mod;
}
void modulo_16(uint32_t declargs, uint16_t *ret, uint16_t div, uint16_t mod, alu_t* alu, SHORT_STDPARAMS) {
    *ret = div % mod;
}

void modulo_32(uint32_t declargs, uint32_t *ret, uint32_t div, uint32_t mod, alu_t* alu, SHORT_STDPARAMS) {
    *ret = div % mod;
}

void modulo_64(uint32_t declargs, uint64_t *ret, uint64_t div, uint64_t mod, alu_t* alu, SHORT_STDPARAMS){
    *ret = div % mod;
}