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
#define ADVANCE_WATERMARK 5
#define REQUEST_MISSING 6

#define X 128 // enforce commit after

/*
struct request_s {
	uint32_t args;
	uint32_t timestamp;
	uint16_t clientId;
	uint8_t req;
	bool delivered;
	bool processed;
} ;

typedef struct request_s request_t;
*/
/*
struct vector {
    void *ptr;
    uint32_t increase_bound;
    uint32_t decrease_bound;
    uint16_t size;
    uint16_t elem_size;

    uint8_t increase_rate;
    uint8_t decrease_rate;

};

typedef struct vector vector_t;

vector_t* init_vector(uint16_t elem_size, uint16_t start_size, uint8_t inc_rate, uint32_t dec_rate) {
    if (inc_rate == 0) {
        inc_rate = 100;
    }
    if (dec_rate == 0) {
        inc_rate = 25;
    }
    if (start_size == 0) {
        start_size = 256;
    }

    vector_t *vec = malloc(sizeof(vector_t));

    vec->size = 0;
    vec->elem_size = elem_size;
    vec->increase_rate = inc_rate;
    vec->decrease_rate = dec_rate;
    vec->increase_bound =  (uint32_t) (inc_rate * start_size / 100);
    vec->decrease_bound =  (uint32_t) (dec_rate * start_size / 100);

    vec->ptr = malloc(start_size * elem_size);
}

void* get_array(vector_t vector) {
    return vector.ptr;
}

void add_element(vector_t *vec, void* elem) {
    memcpy(vec->ptr + vec->size * vec->elem_size, elem, vec->elem_size);
    vec->size++;
    if (vec->size >= vec->increase_bound) {
        vec->size *= 2;
        vec->ptr = realloc(vec->ptr, vec->size);
        vec->decrease_bound = (uint32_t) (vec->size * dec_rate / 100);
        vec->increase_bound = (uint32_t) (vec->size * dec_rate / 100);
    }
}

void add_element(vector_t *vec, void* elem) {
    memcpy(vec->ptr + vec->size * vec->elem_size, elem, vec->elem_size);
    vec->size++;
    if (vec->size >= vec->increase_bound) {
        vec->size *= 2;
        vec->ptr = realloc(vec->ptr, vec->size);
        vec->decrease_bound = (uint32_t) (vec->size * dec_rate / 100);
        vec->increase_bound = (uint32_t) (vec->size * dec_rate / 100);
    }
}
*/

struct request_to_store {
    uint32_t sn;
    uint32_t lv;
    digest_t digest;
    request_t request;
} __attribute__((__packed__)); 

typedef struct request_store request_store_t;
typedef struct request_to_store request_to_store_t;

struct checkpoint {
    uint64_t bitmask;
    uint32_t sn;
    uint32_t lv;
    cp_digest_t digest;
    uint8_t count;
    bool stable;
} __attribute__((__packed__));

typedef struct checkpoint checkpoint_t;

struct request_pack {
    bool committed;
    checkpoint_t *cp;
    request_to_store_t requests[];
};

typedef struct request_pack request_pack_t;


struct request_store {

    struct rte_hash* table;
    struct rte_hash* checkpoints;
    //struct rte_hash* snlv;

    uint32_t max_not_executed;
    uint32_t min_not_executed;

    uint32_t last_checkpoint;

    checkpoint_t *last_stable;
    checkpoint_t *last_unstable;

    uint8_t unstable_checkpoints;
    uint32_t checkpoints_count;

    uint16_t backoff;

    request_pack_t* packs[4][16];

    uint16_t size;

    char filename[50];

    uint8_t nodes;
    uint8_t f;

    uint8_t id;

    bool multithreaded;
};

struct cp_params {
        request_pack_t* pack;
        request_store_t* rs;
};

struct missing_params {
    request_store_t* rs;
    uint32_t sn;
    uint32_t lv;
};

typedef struct cp_params cp_params_t;
typedef struct missing_params missing_params_t;

request_store_t* request_store(uint32_t size, uint8_t nodes, uint8_t id, bool multithreaded, SHORT_STDPARAMS);

void extern_request_store_isDelivered(uint32_t, uint32_t, uint32_t, uint32_t, bool *del, digest_t digest, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getByDigest(uint32_t, uint32_t, uint32_t, uint32_t, uint8_t *req, uint32_t *args, uint32_t *timestamp, uint16_t *clientId, bool *delivered, bool *processed, digest_t digest, bool *found, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_createCheckpoint(uint32_t, uint32_t, uint32_t, uint32_t, cp_digest_t *cp, uint32_t lv, uint32_t sn, uint16_t ID,  request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getCheckpointByDigest(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t *sn, uint32_t *lv, bool *stable, cp_digest_t cp, request_store_t *rs, SHORT_STDPARAMS);
//void extern_request_store_add(uint32_t, uint32_t, uint32_t, uint32_t, digest_t *dig, uint16_t ID, uint32_t timestamp, request_payload_t *request, request_store_t *rs, SHORT_STDPARAMS);
    //void extern_request_store_add_request(uint32_t, uint32_t, uint32_t, uint32_t, digest_t *dig, request_t r,  request_store_t *rs, SHORT_STDPARAMS) {
    //}
void extern_request_store_add_request(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t *dig, uint32_t sn, uint32_t lv, uint8_t req, uint32_t args, uint32_t timestamp, uint16_t clientId, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_updateCheckpoint(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t cp_digest, uint32_t sn, uint32_t lv, uint16_t id, request_store_t *rs, SHORT_STDPARAMS);

    //TODO delivered=true
void extern_request_store_commit(uint32_t, uint32_t, uint32_t, uint32_t, digest_t digest, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getDigest(uint32_t, uint32_t, uint32_t, uint32_t, digest_t *dig, uint8_t req, uint32_t args, uint32_t timestamp, uint16_t clientId, request_store_t *rs, SHORT_STDPARAMS);

//    void extern_request_store_getDigest(uint32_t, uint32_t, uint32_t, uint32_t, digest_t *dig, request_payload_t request,  request_store_t *rs, SHORT_STDPARAMS) {
//    }

void extern_request_store_contains(uint32_t, uint32_t, uint32_t, uint32_t, bool *ret, digest_t digest,  request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_containsSn(uint32_t, uint32_t, uint32_t, uint32_t, bool *ret, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS);
void extern_request_store_getDigestBySn(uint32_t, uint32_t, uint32_t, uint32_t, digest_t *dig, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS);

void extern_request_store_print(uint32_t, uint32_t, uint32_t, uint32_t, uint64_t arg, request_store_t *rs, SHORT_STDPARAMS);
