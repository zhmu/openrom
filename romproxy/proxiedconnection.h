/*
 * Runes of Magic proxy - describes a proxied connection between a local and remote peer
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
#ifndef __PROXIEDCONNECTION__
#define __PROXIEDCONNECTION__

#include "client.h"

class Proxy;

/*! \brief Contains a proxied connection between a local and remote side
 */
class ProxiedConnection : public Client {
public:
	ProxiedConnection(const Address& clientaddr, const Address& remoteaddr);
	virtual ~ProxiedConnection();

	//! \brief Called when the connection is ready (both sides connected)
	virtual void Ready() = 0;

	/*! \brief Called when the local side has something to report
	 *  \returns true on success; false if the connection is to be closed
	 */
	virtual bool OnLocalEvent() = 0;

	/*! \brief Called when the remote side has something to report
	 *  \returns true on success; false if the connection is to be closed
	 */
	virtual bool OnRemoteEvent() = 0;

	//! \brief Retrieves the local client of the proxied connection
	Client& GetLocalClient();

	//! \brief Retrieves the remote client of the proxied connection
	Client& GetRemoteClient();

private:
	//! \brief Remote client in use
	Client* m_remoteclient;
};

inline Client&
ProxiedConnection::GetLocalClient()
{
	return *this;
}

inline Client&
ProxiedConnection::GetRemoteClient()
{
	return *m_remoteclient;
}

#endif /* __PROXIEDCONNECTION__ */
