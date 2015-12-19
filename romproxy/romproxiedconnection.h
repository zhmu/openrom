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
#ifndef __ROMPROTOCOLPROXY_H__
#define __ROMPROTOCOLPROXY_H__

#include "proxiedconnection.h"
#include "romconnectioncallback.h"

class ROMConnection;

class ROMProxiedConnection : public ProxiedConnection {
public:
	ROMProxiedConnection(const Address& clientaddr, const Address& remoteaddr);
	virtual ~ROMProxiedConnection();

	virtual void Ready();
	virtual bool OnLocalEvent();
	virtual bool OnRemoteEvent();

protected:
	virtual void OnLocalNewKey();
	virtual void OnLocalPacket(struct ROM::Packet* p);
	virtual void OnRemoteNewKey();
	virtual void OnRemotePacket(struct ROM::Packet* p);

	virtual void OnLocalRawPacket(const struct ROM::Packet* p);
	virtual void OnRemoteRawPacket(const struct ROM::Packet* p);

private:
	//! \brief Callbacks used for the local connection
	class LocalCallback : public ROMConnectionCallback {
	public:
		LocalCallback(ROMProxiedConnection& connection);

		virtual void OnNewKey();
		virtual void OnPacket(struct ROM::Packet* p);
		virtual void OnRawPacketReceived(const struct ROM::Packet* p);

	protected:
		ROMProxiedConnection& m_Connection;
	};

	//! \brief Callbacks used for the remote connection
	class RemoteCallback : public ROMConnectionCallback {
	public:
		RemoteCallback(ROMProxiedConnection& connection);

		virtual void OnNewKey();
		virtual void OnPacket(struct ROM::Packet* p);
		virtual void OnRawPacketReceived(const struct ROM::Packet* p);

	protected:
		ROMProxiedConnection& m_Connection;
	};

	//! \brief Local connection
	ROMConnection* m_LocalConnection;

	//! \brief Local connection callbacks
	LocalCallback m_LocalCallback;

	//! \brief Remote connection
	ROMConnection* m_RemoteConnection;

	//! \brief Remote connection callbacks
	RemoteCallback m_RemoteCallback;
};

#endif /* __ROMPROTOCOLPROXY_H__ */
