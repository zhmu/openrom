/*
 * Runes of Magic proxy - client
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
#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "address.h"
#include "socket.h"

class Client : public Socket {
	friend class Server;
public:	
	Client();
	Client(const Address& localaddr, const Address& remoteaddr);

	/*! \brief Connects to a given address
	 *  \param address Address to connect to
	 *  \returns true on success
	 */
	bool Connect(const Address& address);

	//! \brief To be called when the client's file descriptor signals
	virtual void OnEvent();

	//! \brief Is the client to be dropped?
	bool MustDrop() const;

	//! \brief Retrieve the local client address
	const Address& GetLocalAddress() const;

	//! \brief Retrieve the remote address
	const Address& GetRemoteAddress() const;

protected:
	//! \brief Sets the file descriptor
	void SetFD(int fd);

	//! \brief Whether the client is to be dropped
	bool m_MustDrop;

	//! \brief Client address
	Address m_LocalAddress;

	//! \brief Remote address
	Address m_RemoteAddress;
};

inline bool
Client::MustDrop() const
{
	return m_MustDrop;
}

inline const Address&
Client::GetLocalAddress() const
{
	return m_LocalAddress;
}

inline const Address&
Client::GetRemoteAddress() const
{
	return m_RemoteAddress;
}

#endif /*  __CLIENT_H__ */
