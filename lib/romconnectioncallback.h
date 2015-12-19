/*
 * Runes of Magic proxy - ROM protocol callbacks
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
#ifndef __ROMCONNECTIONCALLBACK_H__
#define __ROMCONNECTIONCALLBACK_H__

namespace ROM {
	struct Packet;
};

class ROMConnectionCallback {
public:
	//! \brief Called when a new key has been accepted
	virtual void OnNewKey() = 0;

	//! \brief Called when a new packet has been accepted
	virtual void OnPacket(struct ROM::Packet* p) = 0;

	//! \brief Called when a raw, unprocessed complete packet has been received
	virtual void OnRawPacketReceived(const struct ROM::Packet* p) = 0;
};

#endif /* __ROMCONNECTIONCALLBACK_H__ */
