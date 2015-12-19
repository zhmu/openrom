/*
 * Runes of Magic proxy - proxy
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
#ifndef __PROXY_H__
#define __PROXY_H__

#include <sys/select.h>
#include <list>
#include "address.h"
#include "client.h"
#include "proxiedconnection.h"
#include "server.h"

class Server;

//!  \brief Proxies received connections to a given server
class Proxy
{
public:
	Proxy(const Address& local, const Address& remote);
	virtual ~Proxy();

	bool Initialize();

	/*! \brief Fills a fd_set
	 *  \param max_fd Current maximum entry in fds
	 *  \param fds fd_set to fill
	 *  \returns New maximum entry in fds
	 */
	int FillFdSet(int max_fd, fd_set* fds) const;

	/*! \brief Processes entries in the fd_Set
	 *  \returns true on success, false if this item is to be dropped
	 */
	bool ProcessFdSet(const fd_set* fds);

	//! \brief Retrieves the local address to proxy from
	const Address& GetLocalAddress() const;

	//! \brief Retrieves the remote address to proxy to
	const Address& GetRemoteAddress() const;

protected:
	/*! \brief Called to create a new proxied connection
	 *  \param clientaddr Client address
	 *  \param remoteaddr Remote address to proxy to
	 *  \returns New connection to use
	 *
	 *  The proxy will consider itself owner of the new connection and destroy it as needed.
	 */
	virtual ProxiedConnection* CreateConnection(const Address& clientaddr, const Address& remoteaddr) const = 0;

	void AddConnection(ProxiedConnection& connection);


private:
	class ProxyServer : public Server {
	public:
		ProxyServer(Proxy& proxy);

	protected:
		virtual Client* CreateClient(const Address& clientaddr, const Address& remoteaddr) const;
		Proxy& m_proxy;
	};

	//! \brief Source address to proxy from
	Address m_local;

	//! \brief Destination address to proxy to
	Address m_remote;

	//! \brief Server used
	ProxyServer* m_server;

	//! \brief Connections in use
	typedef std::list<ProxiedConnection*> TProxiedConnectionPtrList;
	TProxiedConnectionPtrList m_connections;
};

inline const Address&
Proxy::GetLocalAddress() const
{
	return m_local;
}

inline const Address&
Proxy::GetRemoteAddress() const
{
	return m_remote;
}

#endif /* __PROXY_H__ */
