/*
 * Runes of Magic proxy - proxy server
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
#ifndef __LOGINPROXYSERVER_H__
#define __LOGINPROXYSERVER_H__

#include "server.h"

class Proxy;

class ProxyServer : public Server {
public:
	ProxyServer(Proxy& proxy);

protected:
	virtual Client* CreateClient() const;

	Proxy& m_proxy;
};

#endif /* __PROXYSERVER_H__ */
