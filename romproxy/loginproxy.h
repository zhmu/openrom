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
#ifndef __LOGINPROXY_H__
#define __LOGINPROXY_H__

#include "proxy.h"
#include "romproxiedconnection.h"

class LoginProxy : public Proxy {
public:
	LoginProxy(const Address& source, const Address& dest);

protected:
	class LoginProxiedConnection : public ROMProxiedConnection {
	public:
		LoginProxiedConnection(const Address& clientaddr, const Address& remoteaddr);
		virtual ~LoginProxiedConnection();

		virtual void OnLocalPacket(struct ROM::Packet* p);
		virtual void OnRemotePacket(struct ROM::Packet* p);
	};

	virtual ProxiedConnection* CreateConnection(const Address& clientaddr, const Address& remoteaddr) const;

};

#endif /* __LOGINPROXY_H__ */
