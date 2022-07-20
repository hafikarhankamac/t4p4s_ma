// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

// This file is included directly from `dpdk_tables.c`.

#ifdef T4P4S_PALMTRIE
    void ternary_create(lookup_table_t* t, int socketid)
    {
        struct palmtrie palmtrie;

        //t->table = palmtrie_init(&palmtrie, t->palmtrie_type);
        t->table = palmtrie_init(&palmtrie, PALMTRIE_BASIC); //default PALMTRIE_BASIC implemented
    }
#else
    void ternary_create(lookup_table_t* t, int socketid)
    {
        t->table = naive_ternary_create(t->entry.key_size, t->max_size);
    }
#endif

#ifdef T4P4S_PALMTRIE
    void ternary_add(lookup_table_t* t, uint8_t* key, uint8_t* mask, int priority, uint8_t* value)
    {
        if (t->entry.key_size == 0) return; // don't add lines to keyless tables

        uint8_t* entry = make_table_entry_on_socket(t, value);

        uint8_t tcam_addr[32];
        uint8_t tcam_mask[32];

        (void)memcpy(&tcam_addr[0], key, 32);
        (void)memcpy(&tcam_mask[0], mask, 32);

        addr_t addr_t_key;
        addr_t addr_t_mask;
        u64 d;

        reverse(&tcam_addr[0]);
        reverse(&tcam_mask[0]);

        memset(&addr_t_key, 0, sizeof(addr_t));
        memset(&addr_t_mask, 0, sizeof(addr_t));
        for ( int i = 0; i < (ssize_t)strlen(16); i++ ) {
            d = hex2bin(tcam_addr[i]);
            addr_t_key.a[i >> 4] |= d << ((i & 0xf) << 2);
            d = hex2bin(tcam_mask[i]);
            addr_t_mask.a[i >> 4] |= d << ((i & 0xf) << 2);
        }

        palmtrie_add_data(t->table, addr_t_key, addr_t_mask, priority, entry);

        //addr_t* addr_t_key;
        //addr_t* addr_t_mask;

        //addr_t_key = (addr_t*)key;
        //addr_t_mask = (addr_t*)mask;

        //palmtrie_add_data(t->table, *addr_t_key, *addr_t_mask, priority, entry);

        palmtrie_commit(t->table);
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

        addr_t* addr_t_key;

        addr_t_key = (addr_t*)key;

        u64 ret = palmtrie_lookup(t->table, *addr_t_key);
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
