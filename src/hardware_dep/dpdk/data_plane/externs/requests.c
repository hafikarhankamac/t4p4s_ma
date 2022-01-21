#include "requests.h"

#include <rte_hash_crc.h>

struct rte_hash* hash_create(int socketid, const char* name, uint32_t keylen, rte_hash_function hashfunc, const uint32_t size, const bool has_replicas);

request_store_t* request_store(uint32_t size, SHORT_STDPARAMS)
{
   request_store_t *rs = (request_store_t*) rte_malloc("request_store_t", size * sizeof(uint8_t), 0);

   //TODO socket id, max_size

   struct rte_hash* h = hash_create(0, "rs-table", sizeof(uint32_t), rte_hash_crc, 1000000, false);
   rs->table = h;
   struct rte_hash* i = hash_create(0, "snlv-table", sizeof(uint64_t), rte_hash_crc, 1000000, false);
   rs->snlv = i;
   rs->max_not_executed = 0;
   rs->min_not_executed = 0;

   return rs;
}



uint64_t get_sn_lv_key(uint32_t sn, uint32_t lv) {
    return  ((uint64_t) sn << 32) | lv;
}


uint32_t hash_naive(uint8_t *key, uint8_t length) {
    return rte_hash_crc(key, length, 0xffffffff);
}

void hash_request(request_payload_t *req, uint8_t **hsh) {
   uint8_t key[5];
   key[0] = req->req;
   key[1] = req->args;
   *hsh = hash_naive(key, 5);
}

uint32_t hash_req(request_t *req) {
    return hash_naive(req, sizeof(request_t));
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
   reqpl = &(req->payload);
}

void extern_request_store_createCheckpoint(uint32_t declarg, cp_digest_t *cp, uint32_t lv, uint32_t sn, uint16_t ID,  request_store_t *rs, SHORT_STDPARAMS)
{
}

void extern_request_store_add(uint32_t declarg, digest_t *dig, uint16_t ID, uint32_t timestamp, request_payload_t *request, request_store_t *rs, SHORT_STDPARAMS)
{
}

    //void extern_request_store_add_request(uint32_t declarg, digest_t *dig, request_t r,  request_store_t *rs, SHORT_STDPARAMS) {
    //}

void extern_request_store_add_request(uint32_t declarg, uint32_t *dig, uint16_t clientId, uint32_t sn, uint32_t lv, uint8_t req_cmd, uint32_t args, request_store_t *rs, SHORT_STDPARAMS)
{
    request_to_store_t *req = rte_malloc("request_to_store_t", sizeof(request_to_store_t) + sizeof(request_t), 0);
    req->sn = sn; 
    req->lv = lv;
    req->request.clientId = clientId;
    req->request.payload = req + sizeof(request_to_store_t);
    req->request.payload->req = req_cmd;
    req->request.payload->args = args;
    hash_request(req->request.payload, dig);
    rte_hash_add_key_with_hash_data(rs->table, dig, *dig, req);
    uint64_t snlv = get_sn_lv_key(sn, lv);
    rte_hash_add_key_data(rs->snlv, &snlv, (void*) (uint64_t) *dig);

    rs->max_not_executed = rs->max_not_executed > sn ? rs->max_not_executed : sn;
}

void extern_request_store_updateCheckpoint(uint32_t declarg, cp_id * cp, uint32_t cp_digest, uint16_t checkpoint_id,  request_store_t *rs, SHORT_STDPARAMS)
{

}

//TODO delivered=true
void extern_request_store_commit(uint32_t declarg, digest_t digest, request_store_t *rs, SHORT_STDPARAMS)
{
    request_to_store_t *req;
    rte_hash_lookup_with_hash_data(rs->table, &digest, digest, &req);
    req->request.delivered = true;

    uint32_t lv = req->lv;
    uint32_t max = rs->max_not_executed > req->sn ? rs->max_not_executed : req->sn;

    request_to_store_t *r;
    digest_t *dig;
    for (uint32_t i = rs->min_not_executed; i <= max; i++) {
        uint64_t snlv = get_sn_lv_key(i, lv);
        rte_hash_lookup_data(rs->snlv, &snlv, &dig);
        rte_hash_lookup_with_hash_data(rs->table, dig, dig, &r);
        if (req->request.processed) {
            rs->min_not_executed = r->sn;
        } else {
            raise_event(PROCESS_REQUEST, digest);
            r->request.processed = true;
        }
    }
}

void extern_request_store_getDigest(uint32_t declarg, digest_t *dig, uint8_t req, uint32_t args, request_store_t *rs, SHORT_STDPARAMS)
{
    request_payload_t request = {.req = req, .args = args};
    hash_request(&request, dig);
}

//    void extern_request_store_getDigest(uint32_t declarg, digest_t *dig, request_payload_t request,  request_store_t *rs, SHORT_STDPARAMS) {
//    }

void extern_request_store_contains(uint32_t declarg, bool *ret, digest_t digest,  request_store_t *rs, SHORT_STDPARAMS)
{
   request_t *req;
   *ret = rte_hash_lookup_with_hash_data(rs->table, digest, digest, &req) >= 0;
}

void extern_request_store_containsSn(uint32_t declarg, bool *ret, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS)
{
    digest_t *dig;
    uint64_t snlv = get_sn_lv_key(sn, lv);
    *ret = rte_hash_lookup_data(rs->snlv, &snlv, &dig) >= 0;
}

void extern_request_store_getDigestBySn(uint32_t declarg, digest_t *dig, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS) {
    uint64_t snlv = get_sn_lv_key(sn, lv);
    rte_hash_lookup_data(rs->snlv, &snlv, dig);
}

void extern_request_store_print(uint32_t declarg, uint64_t arg, request_store_t *rs, SHORT_STDPARAMS)
{
    printf("%i", arg);
}
