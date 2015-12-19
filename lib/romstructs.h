/*
 * Open Runes of Magic server - protocol structures
 * Copyright (C) 2013-2015 Rink Springer <rink@rink.nu>
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
#ifndef __ROMSTRUCTS_H__
#define __ROMSTRUCTS_H__

#include <stdint.h>

#define PACKED __attribute__((packed))

namespace ROM {
	//! \brief Basic packet structure, both client and server
	struct Packet {
		uint32_t	p_length;
		uint8_t		p_flag;
#define ROM_PACKET_FLAG_KEY		0x01
#define ROM_PACKET_FLAG_ENCRYPTED	0x02
#define ROM_PACKET_FLAG_ALIVE_REQUEST	0x04
#define ROM_PACKET_FLAG_ALIVE_REPLY	0x08
		uint8_t		p_data_checksum;
		uint8_t		p_header_checksum;
		uint8_t		p_keynum;
		uint32_t	p_seq;
		uint8_t		p_data[0];
	} PACKED;

#define ROM_KEY_LENGTH 10

	//! \brief ROM log file header
	struct LoggerHeader {
		uint32_t	lh_magic;
#define ROM_LOGGER_HEADER_MAGIC_1	0x214d6f52	/* RoM! */
#define ROM_LOGGER_HEADER_MAGIC_2	0x2b4d6f52	/* RoM+ */
	} PACKED;

	//! \brief ROM log file per-packet header
	struct LoggerPacket {
		uint32_t	lp_source_ip;
		uint16_t	lp_source_port;
		uint32_t	lp_dest_ip;
		uint16_t	lp_dest_port;
		uint16_t	lp_len1;
		uint16_t	lp_len2;		/* Only if ROM_LOGGER_HEADER_MAGIC_2 */
	} PACKED;

#define DECLARE_PACKET(v,type) \
	char v##Data[sizeof(ROM::Packet) + sizeof(type)]; \
	struct ROM::Packet* v##Packet = (struct ROM::Packet*)&v##Data[0]; \
	memset(v##Data, 0, sizeof(v##Data)); \
	type* v = (type*)&v##Packet->p_data[0]

#define DECLARE_RAW(v, ...) \
	uint8_t v##RawData[] = { __VA_ARGS__ }; \
	char v##Data[sizeof(ROM::Packet) + sizeof(v##RawData)]; \
	struct ROM::Packet* v##Packet = (struct ROM::Packet*)&v##Data[0]; \
	memset(v##Data, 0, sizeof(v##Data)); \
	memcpy((char*)&v##Packet->p_data[0], (char*)&v##RawData[0], sizeof(v##RawData))

#define SEND_RAW(v) \
	Send(*v##Packet, sizeof(v##Data))
};

#endif /* __ROMSTRUCTS_H__ */
