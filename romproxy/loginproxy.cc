/*
 * Runes of Magic proxy - login proxy
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
#include "loginproxy.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> // for strtoul()
#include <string.h>
#include "../lib/romstructs.h"
#include "romproxy.h"

LoginProxy::LoginProxy(const Address& source, const Address& dest)
	: Proxy(source, dest)
{
}

ProxiedConnection*
LoginProxy::CreateConnection(const Address& clientaddr, const Address& remoteaddr) const
{
	return new LoginProxiedConnection(clientaddr, remoteaddr);
}

LoginProxy::LoginProxiedConnection::LoginProxiedConnection(const Address& clientaddr, const Address& remoteaddr)
	: ROMProxiedConnection(clientaddr, remoteaddr)
{
	if (g_ROMProxy->GetDebugLevel() > 0) {
		char src[64], dst[64];
		clientaddr.ToString(src, sizeof(src));
		remoteaddr.ToString(dst, sizeof(dst));
		printf("LoginProxy: new connection: client=%s remote=%s\n", src, dst);
	}
}

LoginProxy::LoginProxiedConnection::~LoginProxiedConnection()
{
	if (g_ROMProxy->GetDebugLevel() > 0) {
		char src[64], dst[64];
		GetLocalClient().GetLocalAddress().ToString(src, sizeof(src));
		GetLocalClient().GetRemoteAddress().ToString(dst, sizeof(dst));
		printf("LoginProxy: connection lost: client=%s remote=%s\n", src, dst);
	}
}

void
LoginProxy::LoginProxiedConnection::OnLocalPacket(struct ROM::Packet* p)
{
	if (g_ROMProxy->GetDebugLevel() > 2) {
		char src[64], dst[64];
		GetLocalClient().GetLocalAddress().ToString(src, sizeof(src));
		GetLocalClient().GetRemoteAddress().ToString(dst, sizeof(dst));
		printf("LoginProxy::LoginProxiedConnection::OnLocalPacket(): client=%s remote=%s\n", src, dst);
	}
	ROMProxiedConnection::OnLocalPacket(p);
}

void
LoginProxy::LoginProxiedConnection::OnRemotePacket(struct ROM::Packet* p)
{
	if (g_ROMProxy->GetDebugLevel() > 2) {
		char src[64], dst[64];
		GetLocalClient().GetLocalAddress().ToString(src, sizeof(src));
		GetLocalClient().GetRemoteAddress().ToString(dst, sizeof(dst));
		printf("LoginProxy::LoginProxiedConnection::OnRemotePacket(): client=%s remote=%s\n", src, dst);
	}

	/*
	 * If this is a ServerInfo-packet, we'll have to rewrite it so that it points to our server
	 * instead!
	 */
	if (p->p_length == 0x29c && *(uint32_t*)p->p_data == 0x3) {
		const char* name = (const char*)&p->p_data[0xc];
		char* address = (char*)&p->p_data[0x4c];
		uint32_t* port = (uint32_t*)&p->p_data[0x8c];
		if (g_ROMProxy->GetDebugLevel() > 1)
			fprintf(stderr, "LoginProxy::LoginProxiedConnection::OnRemotePacket(): ServerInfo '%s' at %s:%u, rewriting\n",
			 name, address, *port);

		// Resolve the address
		Address server_addr;
		char port_str[16];
		snprintf(port_str, sizeof(port_str), "%u", *port);
		port_str[sizeof(port_str) - 1] = '\0';
		if (!server_addr.Resolve(address, port_str)) {
			fprintf(stderr, "LoginProxy::LoginProxiedConnection::OnRemotePacket(): cannot resolve %s:%s, ignoring\n",
			 address, port_str);
			ROMProxiedConnection::OnRemotePacket(p);
			return;
		}

		// Find a proxy for the server
		Proxy* proxy = g_ROMProxy->GetProxyForAddress(server_addr);
		if (proxy == NULL) {
			fprintf(stderr, "LoginProxy::LoginProxiedConnection::OnRemotePacket(): cannot create proxy for %s:%s, ignoring\n",
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
			fprintf(stderr, "LoginProxy::LoginProxiedConnection::OnRemotePacket(): proxying '%s' to %s:%u\n",
			 name, address, new_port);
	}

	ROMProxiedConnection::OnRemotePacket(p);
}

/* vim:set ts=2 sw=2: */
