/*
 * Runes of Magic proxy - ROM protocol implementation
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
#ifndef __ROMCONNECTION_H__
#define __ROMCONNECTION_H__

#include <stdint.h>

namespace ROM {
	struct Packet;
};

class Buffer;
class Client;
class ROMConnectionCallback;

class ROMConnection {
public:
	ROMConnection(Client& client, ROMConnectionCallback& callback);
	virtual ~ROMConnection();

	/*! \brief Called on a client event
	 *  \returns true on success, false to abort the connection
	 */
	bool OnEvent();

	/*! \brief Copies the key from another connection
	 *  \param conn Connection to copy the key from
	 */
	void CopyKey(const ROMConnection& conn);

	/*! \brief Sends the current encryption key to the client
	 *  \param p Packet to fill
	 *  \param len Packet buffer length
	 *  \returns true on success
	 */
	bool SendKey(struct ROM::Packet* p, int len);

	/*! \brief Sends a packet to the client
	 *  \param p Packet to send
	 *  \returns true on success
	 *
	 *  The packet contents will be altered due to encryption needs.
	 */
	bool SendPacket(struct ROM::Packet* p);

protected:
	/*! \brief Checksums a packet and sends it off
	 *  \param p Packet to send
	 *  \returns true on success
	 */
	bool ChecksumAndSend(struct ROM::Packet* p);

	/*! \brief Sends a keepalive reply
	 *  \returns true on success
	 */
	bool SendKeepaliveReply();

private:
	//! \brief Client we belong to
	Client& m_Client;

	//! \brief Callbacks to use
	ROMConnectionCallback& m_Callback;

	//! \brief Largest packet accepted
	static const unsigned int s_MaxPacketSize = 131072; /* XXX check this */

	//! \brief Local ROM connection
	Buffer* m_Buffer;

	//! \brief Current encryption key
	uint8_t* m_Key;

	//! \brief Current sequence number
	uint32_t m_Sequence;

	//! \brief Do we have an encryption key?
	bool m_HaveKey;
};

#endif /* __ROMCONNECTION_H__ */
