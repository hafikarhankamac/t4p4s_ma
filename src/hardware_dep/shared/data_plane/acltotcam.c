#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

typedef enum {
    ACL_PERMIT,
    ACL_DENY,
} acl_action_t;

typedef enum {
    ACL_IP,
    ACL_ICMP,
    ACL_IGMP,
    ACL_GGP,
    ACL_IPENCAP,
    ACL_ST2,
    ACL_CBT,
    ACL_EGP,
    ACL_IGP,
    ACL_TCP,
    ACL_UDP,
    ACL_GRE,
    ACL_EIGRP,
    ACL_ESP,
    ACL_AH,
    ACL_OSPF,
    ACL_TP,
} acl_proto_t;

#define ACL_ESTABLISHED     (1 << 0)
#define ACL_ACK             (1 << 1)
#define ACL_NOSYN           (1 << 2)

typedef struct {
    uint16_t fin:1;
    uint16_t syn:1;
    uint16_t rst:1;
    uint16_t psh:1;
    uint16_t ack:1;
    uint16_t urg:1;
    uint16_t ece:1;
    uint16_t cwr:1;
    uint16_t ns:1;
    uint16_t rsvd:7;
} tcp_flag;

typedef struct {
    uint8_t proto;
    uint32_t saddr;
    uint32_t daddr;
    uint16_t sport;
    uint16_t dport;
    uint16_t flags;
} __attribute__ ((packed)) acl_ipv4_entry_t;

typedef struct {
    uint8_t data[16];
    uint8_t mask[16];
} acl_tcam_entry_t;

typedef struct {
    int count;
    uint16_t data[32];
    uint16_t mask[32];
} acl_port_range_masks_t;

typedef struct {
    uint32_t prefix;
    uint32_t mask;
} acl_ipv4_mask_t;

typedef struct {
    uint16_t lb;
    uint16_t ub;
} acl_port_range_t;

typedef struct {
    acl_action_t action;
    acl_proto_t proto;
    union {
        acl_ipv4_mask_t ip4;
    } saddr;
    acl_port_range_t sport;
    union {
        acl_ipv4_mask_t ip4;
    } daddr;
    acl_port_range_t dport;
    int flags;
    int priority;
} acl_t;

/*
 * Parse IPv4 address
 */
int
_parse_ipv4addr(acl_ipv4_mask_t *mask, char **tok, const char *sep,
                char **saveptr)
{
    int octets[4];
    int prefixlen;

    if ( NULL == *tok ) {
        *tok = strtok_r(NULL, sep, saveptr);
        if ( NULL == *tok ) {
            return -1;
        }
    }
    sscanf(*tok, "%d.%d.%d.%d/%d", &octets[0], &octets[1], &octets[2],
           &octets[3], &prefixlen);

    mask->prefix = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8)
        | (octets[3]);
    mask->mask = ((1ULL << (32 - prefixlen)) - 1);

    *tok = NULL;

    return 0;
}

/*
 * Parse port
 */
int
_parse_port(acl_port_range_t *range, char **tok, const char *sep,
            char **saveptr)
{
    int port;

    if ( NULL == *tok ) {
        *tok = strtok_r(NULL, sep, saveptr);
        if ( NULL == *tok ) {
            range->lb = 0;
            range->ub = 65535;
            return 0;
        }
    }
    if ( 0 == strcasecmp(*tok, "eq") ) {
        /* Port */
        *tok = strtok_r(NULL, sep, saveptr);
        port = atoi(*tok);
        range->lb = port;
        range->ub = port;
        *tok = NULL;
    } else if ( 0 == strcasecmp(*tok, "lt") ) {
        /* Port */
        *tok = strtok_r(NULL, sep, saveptr);
        port = atoi(*tok);
        range->lb = 0;
        range->ub = port - 1;
        *tok = NULL;
    } else if ( 0 == strcasecmp(*tok, "gt") ) {
        /* Port */
        *tok = strtok_r(NULL, sep, saveptr);
        port = atoi(*tok);
        range->lb = port + 1;
        range->ub = 65535;
        *tok = NULL;
    } else if ( 0 == strcasecmp(*tok, "ge") ) {
        /* Port */
        *tok = strtok_r(NULL, sep, saveptr);
        port = atoi(*tok);
        range->lb = port;
        range->ub = 65535;
        *tok = NULL;

        *tok = strtok_r(NULL, sep, saveptr);
        if ( 0 == strcasecmp(*tok, "le") ) {
            *tok = strtok_r(NULL, sep, saveptr);
            port = atoi(*tok);
            range->ub = port;
            *tok = NULL;
        }
    } else {
        range->lb = 0;
        range->ub = 65535;
    }

    return 0;
}

int
range2mask_rec(acl_port_range_masks_t *masks, acl_port_range_t range,
               uint16_t prefix, uint16_t mask, int b)
{
    int ret;

    if ( prefix >= range.lb && (prefix | mask) <= range.ub ) {
        if ( masks->count >= 32 ) {
            return -1;
        }
        masks->data[masks->count] = prefix;
        masks->mask[masks->count] = mask;
        masks->count++;
        return 0;
    } else if ( (prefix | mask) < range.lb || prefix > range.ub ) {
        return 0;
    } else {
        /* Partial */
    }
    if ( !mask ) {
        /* End of the recursion */
        return 0;
    }

    mask >>= 1;
    /* Left */
    ret = range2mask_rec(masks, range, prefix, mask, b + 1);
    if ( ret < 0 ) {
        return ret;
    }
    /* Right */
    prefix |= (1 << (15 - b));
    ret = range2mask_rec(masks, range, prefix, mask, b + 1);
    if ( ret < 0 ) {
        return ret;
    }
    return 0;
}

int
range2mask(acl_port_range_masks_t *masks, acl_port_range_t range)
{
    int b;
    uint16_t x;
    uint16_t y;
    uint16_t prefix;
    uint16_t mask;

    masks->count = 0;
    for ( b = 0; b < 16; b++ ) {
        x = range.lb & (1 << (15 - b));
        y = range.ub & (1 << (15 - b));
        if ( x != y ) {
            /* The most significant different bit */
            break;
        }
    }
    mask = (1 << (16 - b)) - 1;
    prefix = range.lb & ~mask;

    return range2mask_rec(masks, range, prefix, mask, b);
}

/*
 * Parse an ACL line
 */
int
parse_acl_line(acl_t *acl, char *buf, int lineno)
{
    char *saveptr;
    char *action;
    char *proto;
    char *tok;
    const char *sep = " \t";
    int ret;

    acl->priority = (1 << 20) - lineno;

    /* Action */
    action = strtok_r(buf, sep, &saveptr);
    if ( NULL == action ) {
        return -1;
    } else if ( 0 == strcasecmp(action, "permit") ) {
        acl->action = ACL_PERMIT;
    } else if ( 0 == strcasecmp(action, "deny") ) {
        acl->action = ACL_DENY;
    } else {
        return -1;
    }

    /* Protocol */
    proto = strtok_r(NULL, sep, &saveptr);
    if ( NULL == proto ) {
        return -1;
    } else if ( 0 == strcasecmp(proto, "ip") ) {
        acl->proto = ACL_IP;
    } else if ( 0 == strcasecmp(proto, "tcp") ) {
        acl->proto = ACL_TCP;
    } else if ( 0 == strcasecmp(proto, "udp") ) {
        acl->proto = ACL_UDP;
    } else if ( 0 == strcasecmp(proto, "icmp") ) {
        acl->proto = ACL_ICMP;
    } else if ( 0 == strcasecmp(proto, "gre") ) {
        acl->proto = ACL_GRE;
    } else if ( 0 == strcasecmp(proto, "eigrp") ) {
        acl->proto = ACL_EIGRP;
    } else if ( 0 == strcasecmp(proto, "esp") ) {
        acl->proto = ACL_ESP;
    } else if ( 0 == strcasecmp(proto, "ah") ) {
        acl->proto = ACL_AH;
    } else if ( 0 == strcasecmp(proto, "ospf") ) {
        acl->proto = ACL_OSPF;
    } else if ( 0 == strcasecmp(proto, "tp++") ) {
        acl->proto = ACL_TP;
    } else if ( 0 == strcasecmp(proto, "igmp") ) {
        acl->proto = ACL_IGMP;
    } else if ( 0 == strcasecmp(proto, "ggp") ) {
        acl->proto = ACL_GGP;
    } else if ( 0 == strcasecmp(proto, "ipencap") ) {
        acl->proto = ACL_IPENCAP;
    } else if ( 0 == strcasecmp(proto, "st2") ) {
        acl->proto = ACL_ST2;
    } else if ( 0 == strcasecmp(proto, "cbt") ) {
        acl->proto = ACL_CBT;
    } else if ( 0 == strcasecmp(proto, "egp") ) {
        acl->proto = ACL_EGP;
    } else if ( 0 == strcasecmp(proto, "igp") ) {
        acl->proto = ACL_IGP;
    } else {
        return -1;
    }

    /* Source IP address */
    tok = NULL;
    ret = _parse_ipv4addr(&acl->saddr.ip4, &tok, sep, &saveptr);
    if ( ret < 0 ) {
        return -1;
    }

    /* Source port */
    ret = _parse_port(&acl->sport, &tok, sep, &saveptr);
    if ( ret < 0 ) {
        return -1;
    }

    /* Destination IP address */
    ret = _parse_ipv4addr(&acl->daddr.ip4, &tok, sep, &saveptr);
    if ( ret < 0 ) {
        return -1;
    }

    /* Destination port */
    ret = _parse_port(&acl->dport, &tok, sep, &saveptr);
    if ( ret < 0 ) {
        return -1;
    }

    /* flags */
    if ( NULL == tok ) {
        tok = strtok_r(NULL, sep, &saveptr);
    }
    acl->flags = 0;
    if ( NULL != tok ) {
        if ( 0 == strcasecmp(tok, "established") ) {
            acl->flags = ACL_ESTABLISHED;
            tok = NULL;
        } else if ( 0 == strcasecmp(tok, "ack") ) {
            acl->flags = ACL_ACK;
            tok = NULL;
        } else if ( 0 == strcasecmp(tok, "nosyn") ) {
            acl->flags = ACL_NOSYN;
            tok = NULL;
        } else if ( '#' == *tok )  {
            /* Comment */
            acl->flags = 0;
            return 0;
        } else {
            return -1;
        }
    }

    /* Rest of the tokens */
    if ( NULL == tok ) {
        tok = strtok_r(NULL, sep, &saveptr);
    }
    if ( NULL == tok ) {
        return 0;
    } else if ( '#' == *tok ) {
        /* Comment */
        return 0;
    } else {
        return -1;
    }
}

/*
 * Parse the specified ACL
 */
int
parse_acl(char *strline, acl_tcam_entry_t *tcam_e)
{
    char buf[1024];
    acl_t acl;
    int ret;
    acl_port_range_masks_t sports;
    acl_port_range_masks_t dports;
    int i;
    int j;
    int k;
    acl_ipv4_entry_t *data;
    acl_ipv4_entry_t *mask;
    int flagc = 1;
    tcp_flag flagd[2];
    tcp_flag flagm[2];

    strcpy(&buf[0], strline);

    ret = parse_acl_line(&acl, &buf[0], 1);
    if ( ret < 0 ) {
        return -1;
    }

    /* Source and destination ports */
    ret = range2mask(&sports, acl.sport);
    if ( ret < 0 ) {
        return -1;
    }
    ret = range2mask(&dports, acl.dport);
    if ( ret < 0 ) {
        return -1;
    }

    /* Flags */
    memset(&flagd[0], 0, sizeof(tcp_flag));
    memset(&flagm[0], 0xff, sizeof(tcp_flag));
    memset(&flagd[1], 0, sizeof(tcp_flag));
    memset(&flagm[1], 0xff, sizeof(tcp_flag));
    if ( ACL_ESTABLISHED & acl.flags ) {
        /* Syn */
        flagc = 2;
        flagd[0].ack = 1;
        flagm[0].ack = 0;
        flagd[1].rst = 1;
        flagm[1].rst = 0;
    } else if ( ACL_ACK & acl.flags ) {
        /* Ack */
        flagd[0].ack = 1;
        flagm[0].ack = 0;
    } else if ( ACL_NOSYN & acl.flags ) {
        /* No-syn */
        flagd[0].syn = 0;
        flagm[0].syn = 0;
    }

    /* Build TCAM */
    for ( i = 0; i < sports.count; i++ ) {
        for ( j = 0; j < dports.count; j++ ) {
            for ( k = 0; k < flagc; k++ ) {
                //memset(&tcam_e, 0x0, sizeof(acl_tcam_entry_t));
                data = (acl_ipv4_entry_t *)tcam_e->data;
                mask = (acl_ipv4_entry_t *)tcam_e->mask;
                memset(mask, 0xff, 16);
                switch ( acl.proto ) {
                case ACL_IP:
                    data->proto = 0;
                    mask->proto = 0xff;
                    break;
                case ACL_ICMP:
                    data->proto = 1;
                    mask->proto = 0;
                    break;
                case ACL_TCP:
                    data->proto = 6;
                    mask->proto = 0;
                    break;
                case ACL_UDP:
                    data->proto = 17;
                    mask->proto = 0;
                    break;
                case ACL_GRE:
                    data->proto = 47;
                    mask->proto = 0;
                    break;
                case ACL_EIGRP:
                    data->proto = 88;
                    mask->proto = 0;
                    break;
                case ACL_ESP:
                    data->proto = 50;
                    mask->proto = 0;
                    break;
                case ACL_AH:
                    data->proto = 51;
                    mask->proto = 0;
                    break;
                case ACL_OSPF:
                    data->proto = 89;
                    mask->proto = 0;
                    break;
                case ACL_TP:
                    data->proto = 39;
                    mask->proto = 0;
                    break;
                case ACL_IGMP:
                    data->proto = 2;
                    mask->proto = 0;
                    break;
                case ACL_GGP:
                    data->proto = 3;
                    mask->proto = 0;
                    break;
                case ACL_IPENCAP:
                    data->proto = 4;
                    mask->proto = 0;
                    break;
                case ACL_ST2:
                    data->proto = 5;
                    mask->proto = 0;
                    break;
                case ACL_CBT:
                    data->proto = 7;
                    mask->proto = 0;
                    break;
                case ACL_EGP:
                    data->proto = 8;
                    mask->proto = 0;
                    break;
                case ACL_IGP:
                    data->proto = 9;
                    mask->proto = 0;
                    break;
                }
                data->saddr = htonl(acl.saddr.ip4.prefix);
                mask->saddr = htonl(acl.saddr.ip4.mask);
                data->daddr = htonl(acl.daddr.ip4.prefix);
                mask->daddr = htonl(acl.daddr.ip4.mask);
                //data->sport = htons(sports.data[i]);
                //mask->sport = htons(sports.mask[i]);
                //data->dport = htons(dports.data[j]);
                //mask->dport = htons(dports.mask[j]);
                //data->flags = *(uint16_t *)&flagd[k];
                //mask->flags = *(uint16_t *)&flagm[k];
            }
        }
    }

    return 0;
}
