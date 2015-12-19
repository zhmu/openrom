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
#include "rompacketlogger.h"
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "../lib/romstructs.h"
#include "address.h"

ROMPacketLogger::ROMPacketLogger()
	: m_FD(-1)
{
}

ROMPacketLogger::~ROMPacketLogger()
{
	Close();
}

bool
ROMPacketLogger::Open(const char* fname)
{
	assert(m_FD < 0);
	m_FD = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (m_FD < 0)
		return false;

	struct ROM::LoggerHeader lh;
	lh.lh_magic = ROM_LOGGER_HEADER_MAGIC_2;
	return write(m_FD, &lh, sizeof(lh)) == sizeof(lh);
}

void
ROMPacketLogger::Close()
{
	if (m_FD >= 0)
		close(m_FD);
	m_FD = -1;
}

bool
ROMPacketLogger::Write(const Address& source, const Address& dest, const struct ROM::Packet* p)
{
	uint32_t src_ip, dst_ip;
	uint16_t src_port, dst_port;

	source.GetIPv4Address(src_ip, src_port);
	dest.GetIPv4Address(dst_ip, dst_port);

	struct ROM::LoggerPacket lp;
	lp.lp_source_ip = src_ip;
	lp.lp_source_port = src_port;
	lp.lp_dest_ip = dst_ip;
	lp.lp_dest_port = dst_port;
	lp.lp_len1 = p->p_length & 0xffff;
	lp.lp_len2 = p->p_length >> 16;
	return
	 write(m_FD, &lp, sizeof(lp)) == sizeof(lp) &&
	 write(m_FD, (const void*)p, p->p_length) == p->p_length;
}

/* vim:set ts=2 sw=2: */
