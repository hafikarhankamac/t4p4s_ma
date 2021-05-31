#pragma once

typedef uint64_t digest_t;
typedef uint64_t cp_digest_t;
typedef uint16_t cp_id;

typedef struct {
    uint8_t *anon;
} request_store_t;


    request_store_t* request_store(uint64_t size) {
        return (request_store_t*) rte_malloc("request_store_t", size * sizeof(uint8_t), 0)
    }

    bool isDelivered(request_store_t *rs, digest_t digest) {
        return false; // TODO stub
    }
    request_payload_t getByDigest(request_store_t *rs, digest_t digest) {
        return NULL;
    }

    cp_digest_t createCheckpoint(request_store_t *rs, uint32_t lv, uint64_t sn, uint16_t ID) {
        return 0;
    }

    digest_t add(request_store_t *rs, uint16_t ID, uint64_timestamp, request_payload_t request) {
        return 0;
    }

    digest_t add_request(request_store_t *rs ,request_t r) {
        return 0;
    }

    cp_id updateCheckpoint(request_store_t *rs, uint64_t cp_digest, uint16_t checkpoint_id);

    //TODO delivered=true
    void commit(request_store_t *rs, digest_t digest) {
    }

    digest_t getDigest(request_store_t *rs, uint8_t req, uint64_t args) {
        return 0;
    }

    digest_t getDigest(request_store_t *rs, request_payload_t request) {
        return 0;
    }


	bool contains(request_store_t *rs, digest_t digest) {
	    return false;
	}
}
