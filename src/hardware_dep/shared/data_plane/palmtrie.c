/*_
 * Copyright (c) 2015-2020 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 */

#include "palmtrie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alloca.h>

/*
 * Initialize an instance
 */
struct palmtrie *
palmtrie_init(struct palmtrie *palmtrie, enum palmtrie_type type)
{
    /* Allocate for the data structure when the argument is not NULL, and then
       clear all the variables */
    palmtrie = malloc(sizeof(struct palmtrie));
    if ( NULL == palmtrie ) {
        /* Memory allocation error */
        return NULL;
    }
    (void)memset(palmtrie, 0, sizeof(struct palmtrie));
    palmtrie->allocated = 1;

    /* Set the type and initialize the type-specific data structure */
    switch ( type ) {
    case PALMTRIE_SORTED_LIST:
        /* Sorted list */
        palmtrie->u.sl.head = NULL;
        break;
    case PALMTRIE_BASIC:
        /* Ternary PATRICIA */
        palmtrie->u.tpt.root = NULL;
        break;
    case PALMTRIE_DEFAULT:
        /* Multiway ternary PATRICIA */
        palmtrie->u.mtpt.root = NULL;
        break;
    case PALMTRIE_PLUS:
        /* Multiway ternary PATRICIA */
        palmtrie->u.popmtpt.root = 0;
        palmtrie->u.popmtpt.nodes.nr = 0;
        palmtrie->u.popmtpt.nodes.used = 0;
        palmtrie->u.popmtpt.nodes.ptr = NULL;
        palmtrie->u.popmtpt.mtpt.root = NULL;
        break;
    default:
        /* Unsupported type */
        if ( palmtrie->allocated ) {
            free(palmtrie);
        }
        return NULL;
    }
    palmtrie->type = type;

    return palmtrie;
}

/*
 * Release the instance
 */
void
palmtrie_release(struct palmtrie *palmtrie)
{
    int ret;

    /* Release type-specific variables */
    switch ( palmtrie->type ) {
    case PALMTRIE_SORTED_LIST:
        ret = palmtrie_sl_release(palmtrie);
        break;
    case PALMTRIE_BASIC:
        ret = palmtrie_tpt_release(palmtrie);
        break;
    case PALMTRIE_DEFAULT:
        ret = palmtrie_mtpt_release(palmtrie);
        break;
    case PALMTRIE_PLUS:
        //ret = palmtrie_mtpt_release(palmtrie);
        ret = 0;
        break;
    default:
        return;
    }
    if ( 0 != ret ) {
        return;
    }

    if ( palmtrie->allocated ) {
        free(palmtrie);
    }
}

/*
 * palmtrie_add_data -- add an entry with data for a specified address to the
 * trie
 */
int
//void
palmtrie_add_data(struct palmtrie *palmtrie, addr_t addr, addr_t mask,
                  int priority, /*u64 data*/uint8_t* data)
{
    switch ( palmtrie->type ) {
    case PALMTRIE_SORTED_LIST:
        return palmtrie_sl_add(palmtrie, addr, mask, priority, (void *)data);
    case PALMTRIE_BASIC:
        return palmtrie_tpt_add(palmtrie, addr, mask, priority, (void *)data);
    case PALMTRIE_DEFAULT:
        return palmtrie_mtpt_add(&palmtrie->u.mtpt, addr, mask, priority,
                                 (void *)data);
    case PALMTRIE_PLUS:
        return palmtrie_popmtpt_add(&palmtrie->u.popmtpt, addr, mask, priority,
                                    (void *)data);
    default:
        /* Not supported type */
        return -1;
    }
}

/*
 * palmtrie_lookup -- lookup an entry corresponding to the specified address
 * from the trie
 */
u64
//uint8_t*
palmtrie_lookup(struct palmtrie *palmtrie, addr_t addr)
{
    switch ( palmtrie->type ) {
    case PALMTRIE_SORTED_LIST:
        return (u64)palmtrie_sl_lookup(palmtrie, addr);
        //return (uint8_t*)palmtrie_sl_lookup(palmtrie, addr);
    case PALMTRIE_BASIC:
        return (u64)palmtrie_tpt_lookup(palmtrie, addr);
        //return (uint8_t*)palmtrie_tpt_lookup(palmtrie, addr);
    case PALMTRIE_DEFAULT:
        return (u64)palmtrie_mtpt_lookup(palmtrie, addr);
        //return (uint8_t*)palmtrie_mtpt_lookup(palmtrie, addr);
    case PALMTRIE_PLUS:
        return (u64)palmtrie_popmtpt_lookup(&palmtrie->u.popmtpt, addr);
        //return (uint8_t*)palmtrie_popmtpt_lookup(palmtrie, addr);
    default:
        //return 0;
        return NULL;
    }

    //return 0;
    return NULL;
}

/*
 * palmtrie_commit -- compile an optimized trie by applying incremental updates
 */
int
palmtrie_commit(struct palmtrie *palmtrie)
{
    if ( PALMTRIE_PLUS == palmtrie->type ) {
        return palmtrie_popmtpt_commit(&palmtrie->u.popmtpt);
    }

    return 0;
}

void
palmtrie_reverse(char *s)
{
    char *t;
    int l;
    int i;

    l = strlen(s);
    t = alloca(l + 1);
    
    for ( i = 0; i < l; i++ ) {
        t[i] = s[l - i - 1];
    }
    t[l] = 0;

    strcpy(s, t);
}

int
palmtrie_hex2bin(char c)
{
    if ( c >= '0' && c <= '9' ) {
        return c - '0';
    } else if ( c >= 'a' && c <= 'f' ) {
        return c - 'a' + 10;
    } else if ( c >= 'A' && c <= 'F' ) {
        return c - 'A' + 10;
    } else {
        return 0;
    }
}

void
palmtrie_string2hexstring(char *input, char *output)
{
    int loop;
    int i; 
    
    loop = 0;
    i = 0;
    
    while (input[loop] != '\0') {
        sprintf((char*)(output + i), "%02X", input[loop]);

        loop += 1;

        i += 2;
    }

    output[i++] = '\0';
}

unsigned
palmtrie_char2digit(int ch)
{
    static const char Hex[] = "0123456789ABCDEF0123456789abcdef";
    char *p = memchr(Hex, ch, 32);

    if (p)
        return (unsigned) (p - Hex) % 16;

    return (unsigned)(-1);
}

char *
palmtrie_hexstring2string(char *dest, size_t size, const char *src)
{
    char *p = dest;

    if (size <= 0)
        return NULL;

    size--;

    while (*src) {
        if (size == 0) return NULL;

        size--;

        unsigned msb = palmtrie_char2digit(*src++);

        if (msb > 15) return NULL;

        unsigned lsb = char2digit(*src++);
        
        if (lsb > 15) return NULL;
        
        char ch = (char) (msb * 16 + lsb);

        if (ch == 0) return NULL;

        *p++ = ch;
    }

    *p = '\0';

    return dest;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
