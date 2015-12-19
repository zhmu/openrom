/*
 * Runes of Magic proxy - ROM protocol proxy
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
#include "romproxiedconnection.h"
#include <assert.h>
#include <stdio.h>
#include "../lib/romstructs.h"
#include "romconnection.h"
#include "rompacketlogger.h"
#include "romproxy.h"

/*
 * Data flow is the following:
 *
 *  [client]                   [proxy]                   [server]
 *  OnLocalRaw
 *  ------------------------------>       OnLocalPacket
 *                                 ------------------------>
 *
 *                                    OnRemoteRawPacket
 *  OnRemotePacket               <--------------------------
 *  <---------------------------- 
 *
 */

ROMProxiedConnection::ROMProxiedConnection(const Address& clientaddr, const Address& remoteaddr)
	: ProxiedConnection(clientaddr, remoteaddr),
	  m_LocalCallback(*this), m_RemoteCallback(*this)
{
	m_LocalConnection = new ROMConnection(GetLocalClient(), m_LocalCallback);
	m_RemoteConnection = new ROMConnection(GetRemoteClient(), m_RemoteCallback);
}

void
ROMProxiedConnection::Ready()
{
	if (g_ROMProxy->GetDebugLevel() > 1) {
		char local_src[64], local_dst[64];
		char remote_src[64], remote_dst[64];
		GetLocalClient().GetLocalAddress().ToString(local_src, sizeof(local_src));
		GetLocalClient().GetRemoteAddress().ToString(local_dst, sizeof(local_dst));
		GetRemoteClient().GetLocalAddress().ToString(remote_src, sizeof(remote_src));
		GetRemoteClient().GetRemoteAddress().ToString(remote_dst, sizeof(remote_dst));

		fprintf(stderr, "ROMProxiedConnection::Ready(): local <src %s dst %s>, remote <src %s dst %s>\n",
		 local_src, local_dst, remote_src, remote_dst);
	}
}

ROMProxiedConnection::~ROMProxiedConnection()
{
	delete m_RemoteConnection;
	delete m_LocalConnection;
}

bool
ROMProxiedConnection::OnLocalEvent()
{
	return m_LocalConnection->OnEvent();
}

bool
ROMProxiedConnection::OnRemoteEvent()
{
	return m_RemoteConnection->OnEvent();
}

void
ROMProxiedConnection::OnRemoteNewKey()
{
	if (g_ROMProxy->GetDebugLevel() > 3)
		fprintf(stderr, "ROMProxiedConnection::OnRemoteNewKey()\n");

	char packet[64]; // XXX
	struct ROM::Packet* p = (struct ROM::Packet*)&packet[0];

	// Inform our local side of the key; it is used both ways
	m_LocalConnection->CopyKey(*m_RemoteConnection);
	m_LocalConnection->SendKey(p, sizeof(packet));

	// Need to insert this key in the key stream
	if (g_ROMProxy->MustLogProxyClientTraffic())
		g_ROMProxy->GetLogger()->Write(GetLocalClient().GetRemoteAddress(), GetLocalClient().GetLocalAddress(), p);
}

void
ROMProxiedConnection::OnRemotePacket(struct ROM::Packet* p)
{
	if (g_ROMProxy->GetDebugLevel() > 3)
		fprintf(stderr, "ROMProxiedConnection::OnRemotePacket()\n");
		m_LocalConnection->SendPacket(p);

	if (g_ROMProxy->MustLogProxyClientTraffic())
		g_ROMProxy->GetLogger()->Write(GetLocalClient().GetRemoteAddress(), GetLocalClient().GetLocalAddress(), p);
}

void
ROMProxiedConnection::OnRemoteRawPacket(const struct ROM::Packet* p)
{
	if (g_ROMProxy->GetDebugLevel() > 3)
		fprintf(stderr, "ROMProxiedConnection::OnRemoteRawPacket()\n");

	if (g_ROMProxy->MustLogProxyServerTraffic())
		g_ROMProxy->GetLogger()->Write(GetRemoteClient().GetRemoteAddress(), GetRemoteClient().GetLocalAddress(), p);
}

void
ROMProxiedConnection::OnLocalNewKey()
{
	fprintf(stderr, "ROMProxiedConnection::OnLocalNewKey(): *local* side sent a key?! - it will be ignored\n");
}

void
ROMProxiedConnection::OnLocalPacket(struct ROM::Packet* p)
{
	if (g_ROMProxy->GetDebugLevel() > 3)
		fprintf(stderr, "ROMProxiedConnection::OnLocalPacket()\n");

	m_RemoteConnection->SendPacket(p);
	if (g_ROMProxy->MustLogProxyServerTraffic())
		g_ROMProxy->GetLogger()->Write(GetRemoteClient().GetLocalAddress(), GetRemoteClient().GetRemoteAddress(), p);
}

void
ROMProxiedConnection::OnLocalRawPacket(const struct ROM::Packet* p)
{
	if (g_ROMProxy->GetDebugLevel() > 3)
		fprintf(stderr, "ROMProxiedConnection::OnLocalRawPacket()\n");

	if (g_ROMProxy->MustLogProxyClientTraffic())
		g_ROMProxy->GetLogger()->Write(GetLocalClient().GetLocalAddress(), GetLocalClient().GetRemoteAddress(), p);
}

ROMProxiedConnection::LocalCallback::LocalCallback(ROMProxiedConnection& connection)
	: m_Connection(connection)
{
}

void
ROMProxiedConnection::LocalCallback::OnNewKey()
{
	m_Connection.OnLocalNewKey();
}

void
ROMProxiedConnection::LocalCallback::OnPacket(struct ROM::Packet* p)
{
	m_Connection.OnLocalPacket(p);
}

void
ROMProxiedConnection::LocalCallback::OnRawPacketReceived(const struct ROM::Packet* p)
{
	m_Connection.OnLocalRawPacket(p);
}

ROMProxiedConnection::RemoteCallback::RemoteCallback(ROMProxiedConnection& connection)
	: m_Connection(connection)
{
}

void
ROMProxiedConnection::RemoteCallback::OnNewKey()
{
	m_Connection.OnRemoteNewKey();
}

void
ROMProxiedConnection::RemoteCallback::OnPacket(struct ROM::Packet* p)
{
	m_Connection.OnRemotePacket(p);
}

void
ROMProxiedConnection::RemoteCallback::OnRawPacketReceived(const struct ROM::Packet* p)
{
	m_Connection.OnRemoteRawPacket(p);
}

/* vim:set ts=2 sw=2: */
