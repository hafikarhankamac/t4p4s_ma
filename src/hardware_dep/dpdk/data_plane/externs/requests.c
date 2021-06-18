#include "requests.h"

request_store_t* request_store(uint32_t size, SHORT_STDPARAMS)
{
    request_store_t *rs = (request_store_t*) rte_malloc("request_store_t", size * sizeof(uint8_t), 0);
    rs->table = rte_hash_create();


    return rs;
}

inline uint32_t hash_req(request_t *req) {
    return hash(req, sizeof(request_t));
}

uint32_t hash(uint8_t *key, uint8_t length) {
    uint32_t h = 0;
    for(int i = 0, i < length; i++) {
        h += key[i];
    }

    return h;
}

void extern_request_store_isDelivered(uint32_t declarg, bool *del, digest_t digest, request_store_t *rs, SHORT_STDPARAMS)
{
    request_t *req;
    rte_hash_lookup_with_hash_data(rs->table, digest, digest, &req);
    *del = req->delivered;
}

void extern_request_store_getByDigest(uint32_t declarg, request_payload_t *reqpl, digest_t digest, request_store_t *rs, SHORT_STDPARAMS)
{
    request_t *req;
    rte_hash_lookup_with_hash_data(rs->table, digest, digest, &req);
    memcpy(reqpl, req->payload, sizeof(request_payload_t));
    *reqpl = req->payload;
}

void extern_request_store_createCheckpoint(uint32_t declarg, cp_digest_t *cp, uint32_t lv, uint32_t sn, uint16_t ID,  request_store_t *rs, SHORT_STDPARAMS)
{
}

void extern_request_store_add(uint32_t declarg, digest_t *dig, uint16_t ID, uint32_t timestamp, request_payload_t *request, request_store_t *rs, SHORT_STDPARAMS)
{
}

    //void extern_request_store_add_request(uint32_t declarg, digest_t *dig, request_t r,  request_store_t *rs, SHORT_STDPARAMS) {
    //}

void extern_request_store_add_request(uint32_t declarg, uint32_t *dig, uint32_t sn, uint32_t lv, uint16_t cliendId, uint8_t req, uint32_t args, request_store_t *rs, SHORT_STDPARAMS)
{
    request_t *req = rte_malloc("request_t", sizeof(request_t), 0);
    req->payload = { .req = req, .args = args};
    req->clientId = clientId;
    *dig = hash_req(req);
    rte_add_key_with_hash_data(rs->table, dig, dig, req);
}

void extern_request_store_updateCheckpoint(uint32_t declarg, cp_id * cp, uint32_t cp_digest, uint16_t checkpoint_id,  request_store_t *rs, SHORT_STDPARAMS)
{
}

//TODO delivered=true
void extern_request_store_commit(uint32_t declarg, digest_t digest, request_store_t *rs, SHORT_STDPARAMS)
{
}

void extern_request_store_getDigest(uint32_t declarg, digest_t *dig, uint8_t req, uint32_t args, request_store_t *rs, SHORT_STDPARAMS)
{
    uint8_t key[5];
    key[0] = req;
    key[1] = arg;
    *dig = hash(key, 5);
}

//    void extern_request_store_getDigest(uint32_t declarg, digest_t *dig, request_payload_t request,  request_store_t *rs, SHORT_STDPARAMS) {
//    }

void extern_request_store_contains(uint32_t declarg, bool *ret, digest_t digest,  request_store_t *rs, SHORT_STDPARAMS)
{
    request_t *req;
    *ret = rte_hash_lookup_with_hash_data(rs->table, digest, digest, req);
}
void extern_request_store_containsSn(uint32_t declarg, bool *ret, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS)
{
}

