#pragma once

#include <rte_malloc.h>
#include <rte_hash.h>
#include <rte_hash_crc.h>

#include "backend.h"
#include "dpdk_tables.h"
#include "common.h"

typedef uint32_t digest_t;
typedef uint32_t cp_digest_t;
typedef uint16_t cp_id;

#define CREATE_CHECKPOINT 1
#define PROCESS_REQUEST 2
#define OLD_REQUEST_AFTER_WATERMARK_ADVANCE 3
#define REQUEST_TIMEOUT 4

struct request_store {

    struct rte_hash* table;
    struct rte_hash* snlv;

    uint32_t max_not_executed;
    uint32_t min_not_executed;
};

struct request_to_store {
    request_t request;
    uint32_t sn;
    uint32_t lv;
};

typedef struct request_store request_store_t;
typedef struct request_to_store request_to_store_t;


request_store_t* request_store(uint32_t size, SHORT_STDPARAMS);

void extern_request_store_isDelivered(uint32_t declarg, bool *del, digest_t digest, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getByDigest(uint32_t declarg, request_payload_t *reqpl, digest_t digest, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_createCheckpoint(uint32_t declarg, cp_digest_t *cp, uint32_t lv, uint32_t sn, uint16_t ID,  request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_add(uint32_t declarg, digest_t *dig, uint16_t ID, uint32_t timestamp, request_payload_t *request, request_store_t *rs, SHORT_STDPARAMS);
    //void extern_request_store_add_request(uint32_t declarg, digest_t *dig, request_t r,  request_store_t *rs, SHORT_STDPARAMS) {
    //}
void extern_request_store_add_request(uint32_t declarg, uint32_t *dig, uint16_t clientId, uint32_t sn, uint32_t lv, uint8_t req, uint32_t args, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_updateCheckpoint(uint32_t declarg, cp_id * cp, uint32_t cp_digest, uint16_t checkpoint_id,  request_store_t *rs, SHORT_STDPARAMS);

    //TODO delivered=true
void extern_request_store_commit(uint32_t declarg, digest_t digest, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getDigest(uint32_t declarg, digest_t *dig, uint8_t req, uint32_t args, request_store_t *rs, SHORT_STDPARAMS);

//    void extern_request_store_getDigest(uint32_t declarg, digest_t *dig, request_payload_t request,  request_store_t *rs, SHORT_STDPARAMS) {
//    }

void extern_request_store_contains(uint32_t declarg, bool *ret, digest_t digest,  request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_containsSn(uint32_t declarg, bool *ret, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getDigestBySn(uint32_t declarg, digest_t *dig, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS);

void extern_request_store_print(uint32_t declarg, uint64_t arg, request_store_t *rs, SHORT_STDPARAMS);
