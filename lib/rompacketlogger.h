/*
 * Runes of Magic proxy - packet logging
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
#ifndef __ROMPACKETLOGGER_H__
#define __ROMPACKETLOGGER_H__

namespace ROM {
	class Packet;
};

class Address;

//! \brief Handle packet logging
class ROMPacketLogger {
public:
	ROMPacketLogger();
	~ROMPacketLogger();

	/*! \brief Attempts to create a log file
	 *  \param fname File to log to
	 *  \returns true on success
	 */
	bool Open(const char* fname);

	//! \brief Closes the log file, if any
	void Close();

	/*! \brief Writes a packet to the log
	 *  \parm source Source address
	 *  \parm dest Destination address
	 *  \parm p Packet to store
	 *  \returns true on success
	 */
	bool Write(const Address& source, const Address& dest, const struct ROM::Packet* p);

private:
	int m_FD;
};

#endif /* __ROMPACKETLOGGER_H__ */
