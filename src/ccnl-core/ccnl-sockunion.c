/*
 * @f ccnl-sockunion.c
 * @b CCN lite (CCNL), core source file (internal data structures)
 *
 * Copyright (C) 2011-17, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2017-06-16 created
 */

#include "ccnl-sockunion.h"

int
ccnl_is_local_addr(sockunion *su)
{
    if (!su)
        return 0;
#ifdef USE_UNIXSOCKET
    if (su->sa.sa_family == AF_UNIX)
        return -1;
#endif
#ifdef USE_IPV4
    if (su->sa.sa_family == AF_INET)
        return su->ip4.sin_addr.s_addr == htonl(0x7f000001);
#endif
#ifdef USE_IPV6
    if (su->sa.sa_family == AF_INET6) {
        static const unsigned char loopback_bytes[] =
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
        static const unsigned char mapped_ipv4_loopback[] =
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1 };
        return ((memcmp(su->ip6.sin6_addr.s6_addr, loopback_bytes, 16) == 0) ||
                (memcmp(su->ip6.sin6_addr.s6_addr, mapped_ipv4_loopback, 16) == 0));
    }

#endif
    return 0;
}

char*
ccnl_addr2ascii(sockunion *su)
{
#ifdef USE_UNIXSOCKET
    static char result[256];
#else
    /* each byte requires 2 chars + 1 for the colon/slash + 6 for the protocol + 1 for \0 */
    static char result[(CCNL_MAX_ADDRESS_LEN * 3) + 7];
#endif

    if (!su)
        return CONSTSTR("(local)");

    switch (su->sa.sa_family) {
#ifdef USE_LINKLAYER
    case AF_PACKET: {
        struct sockaddr_ll *ll = &su->linklayer;
        strcpy(result, ll2ascii(ll->sll_addr, ll->sll_halen));
        sprintf(result+strlen(result), "/0x%04x",
            ntohs(ll->sll_protocol));
        return result;
    }
#endif
#ifdef USE_WPAN
    case AF_IEEE802154: {
        struct sockaddr_ieee802154 *wpan = &su->wpan;
        sprintf(result, "PANID:0x%04x", wpan->addr.pan_id);
        switch (wpan->addr.addr_type) {
            case IEEE802154_ADDR_NONE:
                break;
            case IEEE802154_ADDR_SHORT:
                sprintf(result+strlen(result), ",0x%04x", wpan->addr.addr.short_addr);
                break;
            case IEEE802154_ADDR_LONG:
                sprintf(result+strlen(result), ",%s", ll2ascii(wpan->addr.addr.hwaddr, IEEE802154_ADDR_LEN));
                break;
            default:
                strcpy(result+strlen(result), ",UNKNOWN");
                break;
        }
        return result;
    }
#endif
#ifdef USE_IPV4
    case AF_INET:
        sprintf(result, "%s/%u", inet_ntoa(su->ip4.sin_addr),
                ntohs(su->ip4.sin_port));
        return result;
#endif
#ifdef USE_IPV6
    case AF_INET6: {
        char str[INET6_ADDRSTRLEN];
        sprintf(result, "%s/%u", inet_ntop(AF_INET6, su->ip6.sin6_addr.s6_addr, str, INET6_ADDRSTRLEN),
                ntohs(su->ip6.sin6_port));
        return result;
                   }
#endif
#ifdef USE_UNIXSOCKET
    case AF_UNIX:
        strncpy(result, su->ux.sun_path, sizeof(result)-1);
        result[sizeof(result)-1] = 0;
        return result;
#endif
    default:
        break;
    }

    (void) result; // silence compiler warning (if neither USE_LINKLAYER, USE_IPV4, USE_IPV6, nor USE_UNIXSOCKET is set)
    return NULL;
}