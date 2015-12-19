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
#ifndef __GAMEPROXY_H__
#define __GAMEPROXY_H__

#include "proxy.h"
#include "romproxiedconnection.h"

class GameProxy : public Proxy {
public:
	GameProxy(const Address& source, const Address& dest);

protected:
	class GameProxiedConnection : public ROMProxiedConnection {
	public:
		GameProxiedConnection(const Address& clientaddr, const Address& remoteaddr);
		virtual ~GameProxiedConnection();

		virtual void OnLocalPacket(struct ROM::Packet* p);
		virtual void OnRemotePacket(struct ROM::Packet* p);
	};

	virtual ProxiedConnection* CreateConnection(const Address& clientaddr, const Address& remoteaddr) const;

};

#endif /* __GAMEPROXY_H__ */
