#pragma once

#include <rte_malloc.h>
#include "backend.h"
#include "dpdk_tables.h"
#include "common.h"

typedef uint32_t digest_t;
typedef uint32_t cp_digest_t;
typedef uint16_t cp_id;


struct request_store {
    uint8_t *anon;
};

typedef struct request_store request_store_t;
request_store_t* request_store(uint32_t size, SHORT_STDPARAMS);

void extern_request_store_isDelivered(uint32_t declarg, bool *del, digest_t digest, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getByDigest(uint32_t declarg, request_payload_t *reqpl, digest_t digest, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_createCheckpoint(uint32_t declarg, cp_digest_t *cp, uint32_t lv, uint32_t sn, uint16_t ID,  request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_add(uint32_t declarg, digest_t *dig, uint16_t ID, uint32_t timestamp, request_payload_t *request, request_store_t *rs, SHORT_STDPARAMS);
    //void extern_request_store_add_request(uint32_t declarg, digest_t *dig, request_t r,  request_store_t *rs, SHORT_STDPARAMS) {
    //}
void extern_request_store_add_request(uint32_t declarg, uint32_t *dig, uint32_t timestamp, uint16_t cliendId, uint8_t req, uint32_t args, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_updateCheckpoint(uint32_t declarg, cp_id * cp, uint32_t cp_digest, uint16_t checkpoint_id,  request_store_t *rs, SHORT_STDPARAMS);

    //TODO delivered=true
void extern_request_store_commit(uint32_t declarg, digest_t digest, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getDigest(uint32_t declarg, digest_t *dig, uint8_t req, uint32_t args, request_store_t *rs, SHORT_STDPARAMS);

//    void extern_request_store_getDigest(uint32_t declarg, digest_t *dig, request_payload_t request,  request_store_t *rs, SHORT_STDPARAMS) {
//    }

void extern_request_store_contains(uint32_t declarg, bool *ret, digest_t digest,  request_store_t *rs, SHORT_STDPARAMS);

