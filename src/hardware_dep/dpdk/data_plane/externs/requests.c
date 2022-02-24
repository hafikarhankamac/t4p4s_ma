#include <pthread.h>
#include "requests.h"

#include <rte_hash_crc.h>

struct rte_hash* hash_create(int socketid, const char* name, uint32_t keylen, rte_hash_function hashfunc, const uint32_t size, const bool has_replicas);


void* create_checkpoint(void*);

request_store_t* request_store(uint32_t size, uint8_t nodes, uint8_t id, bool multithreaded, SHORT_STDPARAMS)
{
	request_store_t *rs = (request_store_t*) rte_malloc("request_store_t", size * sizeof(uint8_t), 0);

	//TODO socket id, max_size

	struct rte_hash* h = hash_create(0, "rs-table", sizeof(uint32_t), rte_hash_crc, 1000000, false);
	rs->table = h;
	struct rte_hash* i = hash_create(0, "checkpoint-table", sizeof(uint32_t), rte_hash_crc, 1000000, false);
	rs->checkpoints = h;

	rs->max_not_executed = 0;
	rs->min_not_executed = 0;

	rs-> unstable_checkpoints = 0;
	rs->checkpoints_count = 0;

	// fake_checkpoint
	checkpoint_t *fake_cp = rte_malloc("fake-cp", sizeof(checkpoint_t), 0);
	fake_cp->lv = 0;
	fake_cp->sn = 0;

	rs->last_stable = fake_cp;

	rs->nodes = nodes;
	rs->f = (uint8_t) (nodes - 1 / 3);

	rs->id = id;

	strcpy(rs->filename, "message_log.bin");

	for (uint8_t i = 0; i <= 4; i++) {
		for (uint8_t u = 0; u <= 16; u++) {
			rs->packs[i][u] = malloc(sizeof(request_pack_t));
			rs->packs[i][u]->committed = true;
			for (uint8_t y = 0; y < 128; y++) {
				rs->packs[i][u]->requests[y].lv = i+1; // so they wont match during contains check
			}
		}
	}

	rs->multithreaded = multithreaded;

	return rs;
}



uint64_t get_sn_lv_key(uint32_t sn, uint32_t lv) {
	return  ((uint64_t) sn << 32) | lv;
}


uint32_t hash_naive(void *key, uint32_t length) {
	return rte_hash_crc(key, length, 0xffffffff);
}

request_pack_t* getPack(request_store_t *rs, uint32_t sn, uint32_t lv) {
    return (rs->packs[lv % 4][(sn / 128) % 16]);
}

request_to_store_t* getRequestFromPacks(request_store_t *rs, uint32_t sn, uint32_t lv) {
    return &(getPack(rs, sn, lv)->requests[sn % 128]);
}

/*
   void hash_request(request_payload_t *req, uint8_t **hsh) {
   uint8_t key[5];
   key[0] = req->req;
   key[1] = req->args;
 *hsh = hash_naive(key, 5);
 }
 */

uint32_t hash_request(request_t *req) {
	return hash_naive((void*) req, sizeof(request_t));
}

uint32_t hash_pack(request_to_store_t *req) {
    return hash_naive((void*) req, sizeof(request_to_store_t)*128);
}

void extern_request_store_isDelivered(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, bool *del, digest_t digest, request_store_t *rs, SHORT_STDPARAMS)
{
	request_to_store_t *req;
	rte_hash_lookup_with_hash_data(rs->table, &digest, digest, (void**) &req);
	*del = req->request.delivered;
}

void extern_request_store_getByDigest(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, uint8_t *req, uint32_t *args, uint32_t *timestamp, uint16_t *clientId, bool *delivered, bool *processed, digest_t digest, request_store_t *rs, SHORT_STDPARAMS)
{
	request_to_store_t *rts;
	rte_hash_lookup_with_hash_data(rs->table, &digest, digest, (void**) &rts);
	request_t* r = &rts->request;
	//memcpy(reqpl, req->payload, sizeof(request_payload_t));
	*req = r->req;
	*args = r->args;
	*timestamp = r->timestamp;
	*clientId = r->clientId;
	*delivered = r->delivered;
	*processed = r->processed;
}

void extern_request_store_createCheckpoint(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, cp_digest_t *cp, uint32_t lv, uint32_t sn, uint16_t ID,  request_store_t *rs, SHORT_STDPARAMS)
{

}

//void extern_request_store_add(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, digest_t *dig, uint16_t ID, uint32_t timestamp, request_payload_t *request, request_store_t *rs, SHORT_STDPARAMS)
//{
//}

//void extern_request_store_add_request(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, digest_t *dig, request_t r,  request_store_t *rs, SHORT_STDPARAMS) {
//}

void extern_request_store_add_request(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, uint32_t *dig, uint32_t sn, uint32_t lv, uint8_t req, uint32_t args, uint32_t timestamp, uint16_t clientId, request_store_t *rs, SHORT_STDPARAMS)
{
    request_pack_t *pack = getPack(rs, sn, lv);
    if (sn % 128 == 0 && !pack->committed) {
        //error
    } else {
        pack->committed = false;
        request_to_store_t *r = &(pack->requests[sn % 128]);
	    r->sn = sn;
	    r->lv = lv;
	    r->request.req = req;
	    r->request.args = args;
	    r->request.timestamp = timestamp;
	    r->request.clientId = clientId;
	    r->request.delivered = false;
	    r->request.processed = false;
	    memset(&r->request.c, 0, 16);
	    *dig = hash_request(&r->request);
	    r->digest = *dig;
	    rte_hash_add_key_with_hash_data(rs->table, dig, *dig, r);

	    rs->max_not_executed = rs->max_not_executed > sn ? rs->max_not_executed : sn;
    }
}


//TODO delivered=true
void extern_request_store_commit(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, digest_t digest, request_store_t *rs, SHORT_STDPARAMS)
{
	request_to_store_t *req;
	rte_hash_lookup_with_hash_data(rs->table, &digest, digest, (void**) &req);
	req->request.delivered = true;

	uint32_t lv = req->lv;
	uint32_t max = rs->max_not_executed > req->sn ? rs->max_not_executed : req->sn;

	request_to_store_t *r;
	digest_t dig;
	if (req->sn <= rs->min_not_executed) {
		for (uint32_t i = rs->min_not_executed; i <= max; i++) {
			rte_hash_lookup_with_hash_data(rs->table, &dig, dig, (void**) &r);
			request_pack_t *pack = getPack(rs, i, lv);
			r = &(pack->requests[i % 128]);
			if (r->request.processed) {
				rs->min_not_executed = r->sn + 1;
			} else if (r->request.delivered) {
				uint8_t e_id = PROCESS_REQUEST;
				raise_event(&e_id, &r->digest);
				r->request.processed = true;
				rs->min_not_executed = r->sn+1;
				if (r->sn % 128 == 128-1) {
				    // create checkpoint
				    cp_params_t *par = malloc(sizeof(cp_params_t));
				    par->pack = pack;
				    par->rs = rs;

				    if (rs->multithreaded) {
				        pthread_t thread_id;
				        pthread_create(&thread_id, NULL, create_checkpoint, (void*) par);
				    } else {
				        create_checkpoint((void*) par);
				    }
				}
			} else { //undelivered request -> break
				rs->min_not_executed = r->sn;
				break;
			}
		}
	}
}

void extern_request_store_getCheckpointByDigest(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, uint32_t *sn, uint32_t *lv, bool *stable, cp_digest_t digest, request_store_t *rs, SHORT_STDPARAMS) {
    checkpoint_t *cp;
    rte_hash_lookup_with_hash_data(rs->table, &digest, digest, (void**) &cp);

    *sn = cp->sn;
    *lv = cp->lv;
    *stable = cp->stable;
}


void* create_checkpoint(void *p) {
    cp_params_t *par = (cp_params_t*) p;
    request_pack_t* pack = par->pack;
    request_store_t* rs = par->rs;

    cp_digest_t dig = hash_pack(pack->requests);
    pack->committed = false;

    checkpoint_t *cp;
    int ret = rte_hash_lookup_with_hash_data(rs->checkpoints, &dig, dig, &cp);

    if (ret < 0) {
        cp = (checkpoint_t*) rte_malloc("checkpoint_t", sizeof(checkpoint_t), 0);
        cp->digest = dig;
        cp->stable = false;
        cp->sn = pack->requests[128-1].sn;
        cp->lv = pack->requests[128-1].lv;
        rs->checkpoints_count++;
        rs->unstable_checkpoints++;

        rte_hash_add_key_with_hash_data(rs->checkpoints, &dig, dig, cp);
    } else {
        if (cp->digest == dig) {
            update_bitmask_cp(cp, rs->id);

            if (cp->count >= rs->f) {
                commit_checkpoint(rs, cp);
            }
        }
    }

    uint8_t e_id = CREATE_CHECKPOINT;
    raise_event(&e_id, &dig);

    free(p);
    return NULL;
}

void update_bitmask_cp(checkpoint_t *cp, uint16_t id) {
    if ((cp->bitmask & 1 << id) == 0) {
        cp->count++;
        cp->bitmask |= (1 << id);
    }
}


void commit_checkpoint(request_store_t *rs, checkpoint_t *cp) {
    if (cp->count >= rs->f) {
        uint32_t lv = cp->lv;
        uint32_t sn = cp->sn;
        request_pack_t *pack = getPack(rs, sn, lv);
        FILE *f = fopen(rs->filename, "wb+");
        fwrite(pack->requests, sizeof(request_to_store_t), 128, f);
        fclose(f);
        cp->stable = true;
        pack->committed = true;
    	rs->unstable_checkpoints--;
        rs->last_stable = cp;
        uint8_t e_id = ADVANCE_WATERMARK;
        raise_event(&e_id, &sn);
    }
}

void extern_request_store_updateCheckpoint(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, uint32_t digest, uint32_t sn, uint32_t lv, uint16_t id, request_store_t *rs, SHORT_STDPARAMS)
{
    checkpoint_t *cp;
    int ret = rte_hash_lookup_with_hash_data(rs->table, &digest, digest, (void**) &cp);

    if (ret < 0) {// not found
        if (sn > rs->last_stable->sn && lv >= rs->last_stable->lv) {
            checkpoint_t *cp = (checkpoint_t*) rte_malloc("checkpoint_t", sizeof(checkpoint_t), 0);
            cp->bitmask = 1 << id;
            cp->sn = sn;
            cp->lv = lv;
            cp->count = 1;
            cp->stable = false;
            digest = digest;
            rte_hash_add_key_with_hash_data(rs->table, &digest, digest, cp);
        } else {
            // error
        }
    } else {
        if (cp->sn == sn && cp->lv == lv) {
            update_bitmask_cp(cp, id);

            if (cp->count >= rs->f) {
                commit_checkpoint(rs, cp);
            }
        }
    }
}

void extern_request_store_getDigest(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, digest_t *dig, uint8_t req, uint32_t args, uint32_t timestamp, uint16_t clientId, request_store_t *rs, SHORT_STDPARAMS)
{
	volatile request_t request = {
		.req = req,
		.args = args,
		.timestamp = timestamp,
		.clientId = clientId,
		.delivered = false,
		.processed = false,
		.c = { 0, 0, 0, 0, 0, 0, 0, 0},
	};
	*dig = hash_request(&request);
}

//    void extern_request_store_getDigest(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, digest_t *dig, request_payload_t request,  request_store_t *rs, SHORT_STDPARAMS) {
//    }


void extern_request_store_containsSn(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, bool *ret, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS)
{
	request_to_store_t *r = getRequestFromPacks(rs, sn, lv);
	*ret = r->sn == sn && r->lv == lv;
}

void extern_request_store_getDigestBySn(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, digest_t *dig, uint32_t sn, uint32_t lv, request_store_t *rs, SHORT_STDPARAMS) {
	request_to_store_t *r = getRequestFromPacks(rs, sn, lv);
	*dig = r->digest;
}

void extern_request_store_print(uint32_t declarg, uint32_t declarg2, uint32_t declarg3, uint32_t declarg4, uint64_t arg, request_store_t *rs, SHORT_STDPARAMS)
{
	printf("%i", arg);
}

