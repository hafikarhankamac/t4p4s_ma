// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

// This file is included directly from `dpdk_tables.c`.

#ifdef T4P4S_PALMTRIE
    void ternary_create(lookup_table_t* t, int socketid)
    {
        struct palmtrie palmtrie;

        t->table = palmtrie_init(&palmtrie, PALMTRIE_BASIC); //default PALMTRIE_BASIC implemented
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

        
        char strline[256], *strlineptr;
        acl_tcam_entry_t tcam_e;
        char edata[256], *edataptr;
        char emask[256], *emaskptr;

        strlineptr = &strline[0];
        sprintf(strlineptr, "%s %hhd.%hhd.%hhd.%hhd/%hhd", "permit ip 0.0.0.0/0 ", *key, *(key++), *(key++), *(key++), mask); // ACL like "permit ip 0.0.0.0/0 20.0.1.0/24"

        /*
        if (parse_acl(strline, &tcam_e) == (-1)) return;

        edataptr = &edata[0];
        emaskptr = &emask[0];

        sprintf(edataptr, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                           tcam_e.data[0], tcam_e.data[1], tcam_e.data[2], tcam_e.data[3], tcam_e.data[4], tcam_e.data[5], tcam_e.data[6], tcam_e.data[7],
                           tcam_e.data[8], tcam_e.data[9], tcam_e.data[10], tcam_e.data[11], tcam_e.data[12], tcam_e.data[13], tcam_e.data[14], tcam_e.data[15]);
        sprintf(emaskptr, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                           tcam_e.mask[0], tcam_e.mask[1], tcam_e.mask[2], tcam_e.mask[3], tcam_e.mask[4], tcam_e.mask[5], tcam_e.mask[6], tcam_e.mask[7],
                           tcam_e.mask[8], tcam_e.mask[9], tcam_e.mask[10], tcam_e.mask[11], tcam_e.mask[12], tcam_e.mask[13], tcam_e.mask[14], tcam_e.mask[15]);
        */

        addr_t addr_t_key;
        addr_t addr_t_mask;
        u64 temp;

        /*
        palmtrie_reverse(edata);
        palmtrie_reverse(emask);

        memset(&addr_t_key, 0, sizeof(addr_t));
        memset(&addr_t_mask, 0, sizeof(addr_t));

        for ( int i = 0; i < (ssize_t)strlen(edata); i++ ) {
            temp = palmtrie_hex2bin(edata[i]);
            addr_t_key.a[i >> 4] |= temp << ((i & 0xf) << 2);
            temp = palmtrie_hex2bin(emask[i]);
            addr_t_mask.a[i >> 4] |= temp << ((i & 0xf) << 2);
        }
        */

        addr_t_key[0] = 0x0000000000000000;
        addr_t_mask[0] = 0xFFFFFFFFFFFFFFFF;
        addr_t_key[1] = 0x0000000000140001;
        addr_t_mask[1] = 0xFFFFFFFFFF000000;
        addr_t_key[2] = 0x0000000000000000;
        addr_t_mask[2] = 0x0000000000000000;
        addr_t_key[3] = 0x0000000000000000; 
        addr_t_mask[3] = 0x0000000000000000;
        addr_t_key[4] = 0x0000000000000000; 
        addr_t_mask[4] = 0x0000000000000000;
        addr_t_key[5] = 0x0000000000000000;
        addr_t_mask[5] = 0x0000000000000000;
        addr_t_key[6] = 0x0000000000000000;
        addr_t_mask[6] = 0x0000000000000000;
        addr_t_key[7] = 0x0000000000000000;
        addr_t_mask[7] = 0x0000000000000000;

        //palmtrie_add_data(t->table, addr_t_key, addr_t_mask, priority, entry);
        palmtrie_add_data(t->table, addr_t_key, addr_t_mask, 1, entry);

        palmtrie_commit(t->table); // PALMTRIE_PLUS implemented
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

        //addr_t* addr_t_key;
        //addr_t_key = (addr_t*)key;

        addr_t addr_t_key;
        addr_t_key.a[0] = (*key << 24) | (*(key++) << 16) | (*(key++) << 8) | (*(key++)); 

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
