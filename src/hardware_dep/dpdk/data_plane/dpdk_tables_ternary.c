// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

// This file is included directly from `dpdk_tables.c`.

#ifdef T4P4S_PALMTRIE
    void ternary_create(lookup_table_t* t, int socketid)
    {
        struct palmtrie palmtrie;

        //t->table = palmtrie_init(&palmtrie, PALMTRIE_BASIC); // default PALMTRIE_BASIC is implemented
        t->table = palmtrie_init(&palmtrie, PALMTRIE_PLUS); // default PALMTRIE_PLUS is implemented
    }
#else
    void ternary_create(lookup_table_t* t, int socketid)
    {
        t->table = naive_ternary_create(t->entry.key_size, t->max_size);
    }
#endif

#ifdef T4P4S_PALMTRIE
    //void ternary_add(lookup_table_t* t, uint8_t* key, uint8_t* mask, int priority, uint8_t* value)
    void ternary_add(lookup_table_t* t, uint8_t* key, uint8_t* mask, uint8_t* value)
    {
        if (t->entry.key_size == 0) return; // don't add lines to keyless tables

        uint8_t* entry = make_table_entry_on_socket(t, value);

        addr_t addr_t_key = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
        addr_t addr_t_mask = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
        u64* temp_ptr;
        // for IPv4
        /*
        char strline[MAX_FIELD_LENGTH];
        acl_tcam_entry_t tcam_e;
        char edata[MAX_FIELD_LENGTH];
        char emask[MAX_FIELD_LENGTH];
        u64 temp;

        memset(&edata[0], 0, MAX_FIELD_LENGTH);
        memset(&emask[0], 0, MAX_FIELD_LENGTH);

        sprintf(&strline[0], "%s %hhd.%hhd.%hhd.%hhd/%hhd", "permit ip 0.0.0.0/0", *(key), *(key+1), *(key+2), *(key+3), *mask); // ACL like "permit ip 0.0.0.0/0 10.0.1.0/24"

        //RTE_LOG(INFO, USER1, "strline = %s\n", strline);

        if (parse_acl(&strline[0], &tcam_e) == (-1)) return t->default_val;

        sprintf(&edata[0], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                           tcam_e.data[0], tcam_e.data[1], tcam_e.data[2], tcam_e.data[3], tcam_e.data[4], tcam_e.data[5], tcam_e.data[6], tcam_e.data[7],
                           tcam_e.data[8], tcam_e.data[9], tcam_e.data[10], tcam_e.data[11], tcam_e.data[12], tcam_e.data[13], tcam_e.data[14], tcam_e.data[15]);
        sprintf(&emask[0], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                           tcam_e.mask[0], tcam_e.mask[1], tcam_e.mask[2], tcam_e.mask[3], tcam_e.mask[4], tcam_e.mask[5], tcam_e.mask[6], tcam_e.mask[7],
                           tcam_e.mask[8], tcam_e.mask[9], tcam_e.mask[10], tcam_e.mask[11], tcam_e.mask[12], tcam_e.mask[13], tcam_e.mask[14], tcam_e.mask[15]);

        palmtrie_reverse(&edata[0]);
        palmtrie_reverse(&emask[0]);

        for ( int i = 0; i < (ssize_t)strlen(edata); i++ ) {
            temp = palmtrie_hex2bin(edata[i]);
            addr_t_key.a[i >> 4] |= temp << ((i & 0xf) << 2);
            temp = palmtrie_hex2bin(emask[i]);
            addr_t_mask.a[i >> 4] |= temp << ((i & 0xf) << 2);
        }
        */

        // for Bits
        //uint8_t* tmpkey = key;
        //uint8_t* tmpmask = mask;

        //for ( int i = 0; i < 16; i++ ) {
        //    RTE_LOG(INFO, USER1, "After reverse key[%d]: %hhd mask[%d]: %hhd\n", i, *key, i, *mask);
        //    key++;
        //    mask++;
        //}
        //for ( int i = 0; i < t->entry.key_size; i++ )
        //    RTE_LOG(INFO, USER1, "After reverse data[%d]: 0x%02X mask[%d]: 0x%02X\n", i, edata[i], i, emask[i]);

        //key = tmpkey;
        //mask = tmpmask;
        
        temp_ptr = key;
        addr_t_key.a[0] = *(temp_ptr + 1);
        addr_t_key.a[1] = *(temp_ptr);

        temp_ptr = mask;
        addr_t_mask.a[0] = *(temp_ptr + 1);
        addr_t_mask.a[1] = *(temp_ptr);

        //for ( int i = 0; i < 8; i++ )
        //    RTE_LOG(INFO, USER1, "Add addr_t_key[%d]: 0x%.16lX addr_t_mask[%d]: 0x%.16lX\n", i, addr_t_key.a[i], i, addr_t_mask.a[i]);

        //palmtrie_add_data(t->table, addr_t_key, addr_t_mask, priority, entry);
        palmtrie_add_data(t->table, addr_t_key, addr_t_mask, 1, entry);

        palmtrie_commit(t->table); // if PALMTRIE_PLUS is implemented
    }
#else
    void ternary_add(lookup_table_t* t, uint8_t* key, uint8_t* mask, uint8_t* value)
    {
        if (t->entry.key_size == 0) return; // don't add lines to keyless tables

        uint8_t* entry = make_table_entry_on_socket(t, value);
        naive_ternary_add(t->table, key, mask, entry);
    }
#endif

#ifdef T4P4S_PALMTRIE
    uint8_t* ternary_lookup(lookup_table_t* t, uint8_t* key)
    {
        if (t->entry.key_size == 0) return t->default_val;

        /*
        char strline[256];
        acl_tcam_entry_t tcam_e;
        char edata[256];

        sprintf(&strline[0], "%s %hhd.%hhd.%hhd.%hhd/%hhd", "permit ip 0.0.0.0/0", *(key), *(key+1), *(key+2), *(key+3), 32); // ACL like "permit ip 0.0.0.0/0 10.0.1.2/32"

        //RTE_LOG(INFO, USER1, "strline = %s\n", strline);

        if (parse_acl(&strline[0], &tcam_e) == (-1)) return t->default_val;

        sprintf(&edata[0], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                           tcam_e.data[0], tcam_e.data[1], tcam_e.data[2], tcam_e.data[3], tcam_e.data[4], tcam_e.data[5], tcam_e.data[6], tcam_e.data[7],
                           tcam_e.data[8], tcam_e.data[9], tcam_e.data[10], tcam_e.data[11], tcam_e.data[12], tcam_e.data[13], tcam_e.data[14], tcam_e.data[15]);

        addr_t addr_t_key = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
        u64 temp;

        palmtrie_reverse(&edata[0]);

        for ( int i = 0; i < (ssize_t)strlen(edata); i++ ) {
            temp = palmtrie_hex2bin(edata[i]);
            addr_t_key.a[i >> 4] |= temp << ((i & 0xf) << 2);
        }
        */

        addr_t addr_t_key = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
        u64* key_ptr = (u64*)key;

        addr_t_key.a[0] = *key_ptr;
        addr_t_key.a[1] = *(key_ptr + 1);

        for ( int i = 0; i < 8; i++ )
           RTE_LOG(INFO, USER1, "Lookup addr_t_key[%d]: 0x%.16lX\n", i, addr_t_key.a[i]);

        u64 ret = palmtrie_lookup(t->table, addr_t_key);
        return (uint8_t*)ret == NULL ? t->default_val : (uint8_t*)ret;
    }
#else
    uint8_t* ternary_lookup(lookup_table_t* t, uint8_t* key)
    {
        if (t->entry.key_size == 0) return t->default_val;

        uint8_t* ret = naive_ternary_lookup(t->table, key);
        return ret == NULL ? t->default_val : ret;
    }
#endif

#ifdef T4P4S_PALMTRIE
    void ternary_flush(lookup_table_t* t)
    {
        if (t->entry.key_size == 0) return; // nothing must have been added

        palmtrie_release(t->table);
    }
#else
    void ternary_flush(lookup_table_t* t)
    {
        if (t->entry.key_size == 0) return; // nothing must have been added

        naive_ternary_flush(t->table);
    }
#endif
