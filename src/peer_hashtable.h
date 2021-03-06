/*
  Copyright (c) 2012-2015, Matthias Schiffer <mschiffer@universe-factory.net>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
   \file

   A hashtable allowing fast lookup from an IP address to a peer
*/


#pragma once


#include "hash.h"
#include "peer.h"


/** Hashes a peer address */
static inline void fastd_peer_address_hash(uint32_t *hash, const fastd_peer_address_t *addr) {
	switch(addr->sa.sa_family) {
	case AF_INET:
		fastd_hash(hash, &addr->in.sin_addr.s_addr, sizeof(addr->in.sin_addr.s_addr));
		fastd_hash(hash, &addr->in.sin_port, sizeof(addr->in.sin_port));
		break;

	case AF_INET6:
		fastd_hash(hash, &addr->in6.sin6_addr, sizeof(addr->in6.sin6_addr));
		fastd_hash(hash, &addr->in6.sin6_port, sizeof(addr->in6.sin6_port));
		if (IN6_IS_ADDR_LINKLOCAL(&addr->in6.sin6_addr))
			fastd_hash(hash, &addr->in6.sin6_scope_id, sizeof(addr->in6.sin6_scope_id));
		break;

	default:
		exit_bug("peer_address_bucket: unknown address family");
	}
}


void fastd_peer_hashtable_init(void);
void fastd_peer_hashtable_free(void);

void fastd_peer_hashtable_insert(fastd_peer_t *peer);
void fastd_peer_hashtable_remove(fastd_peer_t *peer);
fastd_peer_t *fastd_peer_hashtable_lookup(const fastd_peer_address_t *addr);
