/*
 * Runes of Magic proxy - game proxy
 * Copyright (C) 2014-2015 Rink Springer <rink@rink.nu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "gameproxy.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> // for strtoul()
#include <string.h>
#include "../lib/romstructs.h"
#include "romproxy.h"

GameProxy::GameProxy(const Address& source, const Address& dest)
	: Proxy(source, dest)
{
}

ProxiedConnection*
GameProxy::CreateConnection(const Address& clientaddr, const Address& remoteaddr) const
{
	return new GameProxiedConnection(clientaddr, remoteaddr);
}

GameProxy::GameProxiedConnection::GameProxiedConnection(const Address& clientaddr, const Address& remoteaddr)
	: ROMProxiedConnection(clientaddr, remoteaddr)
{
	if (g_ROMProxy->GetDebugLevel() > 0) {
		char src[64], dst[64];
		clientaddr.ToString(src, sizeof(src));
		remoteaddr.ToString(dst, sizeof(dst));
		printf("GameProxy: new connection: client=%s remote=%s\n", src, dst);
	}
}

GameProxy::GameProxiedConnection::~GameProxiedConnection()
{
	if (g_ROMProxy->GetDebugLevel() > 0) {
		char src[64], dst[64];
		GetLocalClient().GetLocalAddress().ToString(src, sizeof(src));
		GetLocalClient().GetRemoteAddress().ToString(dst, sizeof(dst));
		printf("GameProxy: connection lost: client=%s remote=%s\n", src, dst);
	}
}

void
GameProxy::GameProxiedConnection::OnLocalPacket(struct ROM::Packet* p)
{
	if (g_ROMProxy->GetDebugLevel() > 2) {
		char src[64], dst[64];
		GetLocalClient().GetLocalAddress().ToString(src, sizeof(src));
		GetLocalClient().GetRemoteAddress().ToString(dst, sizeof(dst));
		printf("GameProxy::GameProxiedConnection::OnLocalPacket(): client=%s remote=%s\n", src, dst);
	}

	ROMProxiedConnection::OnLocalPacket(p);
}

void
GameProxy::GameProxiedConnection::OnRemotePacket(struct ROM::Packet* p)
{
	if (g_ROMProxy->GetDebugLevel() > 2) {
		char src[64], dst[64];
		GetLocalClient().GetLocalAddress().ToString(src, sizeof(src));
		GetLocalClient().GetRemoteAddress().ToString(dst, sizeof(dst));
		printf("GameProxy::GameProxiedConnection::OnRemotePacket(): client=%s remote=%s\n", src, dst);
	}

	/*
	 * If this is a Redirect-packet, we'll have to rewrite it so that it points
	 * to our server instead!
	 */
	if (p->p_length == 0x90 && *(uint32_t*)p->p_data == 0x78) {
		char* address = (char*)&p->p_data[0xc];
		uint32_t* port = (uint32_t*)&p->p_data[0x2c];
		if (g_ROMProxy->GetDebugLevel() > 1)
			fprintf(stderr, "GameProxy::GameProxiedConnection::OnRemotePacket(): Redirect to %s:%u, rewriting\n",
			 address, *port);

		// Resolve the address
		Address server_addr;
		char port_str[16];
		snprintf(port_str, sizeof(port_str), "%u", *port);
		port_str[sizeof(port_str) - 1] = '\0';
		if (!server_addr.Resolve(address, port_str)) {
			fprintf(stderr, "GameProxy::GameProxiedConnection::OnRemotePacket(): cannot resolve %s:%s, ignoring\n",
			 address, port_str);
			ROMProxiedConnection::OnRemotePacket(p);
			return;
		}

		// Find a proxy for the server
		Proxy* proxy = g_ROMProxy->GetProxyForAddress(server_addr);
		if (proxy == NULL) {
			fprintf(stderr, "GameProxy::GameProxiedConnection::OnRemotePacket(): cannot create proxy for %s:%s, ignoring\n",
			 address, port_str);
			ROMProxiedConnection::OnRemotePacket(p);
			return;
		}

		char proxy_addr[256];
		proxy->GetLocalAddress().ToString(proxy_addr, sizeof(proxy_addr));
		char* separator = strchr(proxy_addr, ':');
		assert(separator != NULL);
		*separator = '\0';
		unsigned int new_port = strtoul(separator + 1, NULL, 10);

		strcpy(address, proxy_addr);
		*port = new_port;

		if (g_ROMProxy->GetDebugLevel() > 0)
			fprintf(stderr, "GameProxy::GameProxiedConnection::OnRemotePacket(): proxying to %s:%u\n",
			 address, new_port);
	}

	ROMProxiedConnection::OnRemotePacket(p);
}

/* vim:set ts=2 sw=2: */
