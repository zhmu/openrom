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
#include "romconnection.h"
#include <assert.h>
#include <stdio.h>
#include <string.h> // for memset()
#include "../lib/romstructs.h"
#include "client.h"
#include "buffer.h"
#include "romconnectioncallback.h"

ROMConnection::ROMConnection(Client& client, ROMConnectionCallback& callback)
	: m_Client(client), m_Callback(callback)
{
	m_Buffer = new Buffer;
	m_Key = new uint8_t[ROM_KEY_LENGTH];
	memset(m_Key, 0, ROM_KEY_LENGTH);
	m_HaveKey = false;
	m_Sequence = 1;
}

ROMConnection::~ROMConnection()
{
	delete[] m_Key;
	delete m_Buffer;
}

bool
ROMConnection::OnEvent()
{
	// First of all, grow the buffer with whatever we have
	{
		uint8_t packet[s_MaxPacketSize];
		int len = m_Client.Read(packet, sizeof(packet));
		if (len <= 0)
			return false; // read error; likely socket closed

		if (!m_Buffer->AddData(packet, len)) {
			fprintf(stderr, "ROMProxiedConnection::OnLocalEvent(): out of buffer space, closing connection\n");
			return false; // out of buffer space
		}
	}

	// Now keep processing the data while we can
	while(true) {
		// Attempt to grab a header; if this isn't available, we can't do much
		struct ROM::Packet* p = (struct ROM::Packet*)m_Buffer->PeekData(sizeof(struct ROM::Packet));
		if (p == NULL)
			break;

		// Sanity check
		if (p->p_length < sizeof(*p) || p->p_length >= s_MaxPacketSize) {
			fprintf(stderr, "ROMConnection::OnEvent(): invalid or excessive packet length %d, giving up\n", p->p_length);
			return false;
		}

		// See if the entire packet is available
		if (m_Buffer->PeekData(p->p_length) == NULL)
			break; // better luck next time

		// We have an entire packet - send it to the logger
		int data_length = p->p_length - sizeof(*p);
		m_Callback.OnRawPacketReceived(p);

		if (p->p_flag & ROM_PACKET_FLAG_KEY) {
			// This is a key packet (i.e. it will tell us the encryption key to use)
			if (data_length != ROM_KEY_LENGTH) {
				fprintf(stderr, "ROMConnection::OnEvent(): key packet with invalid length %d, ignoring\n", data_length);
				m_Buffer->FlushData(p->p_length);
				continue;
			}

			// Copy the key to our connection; it will be used from the next packet onwards
			for (unsigned int n = 0; n < ROM_KEY_LENGTH; n++)
				m_Key[n] = (p->p_data[n] + 8) ^ 8;
			m_HaveKey = true;
			m_Callback.OnNewKey();
			m_Buffer->FlushData(p->p_length);
			continue;
		}

		// Verify the header checksum
		bool bHeaderChecksumOK = false;
		{
			/* Verify header checksum */
			uint8_t cksum = 0;
			for (unsigned int n = 0; n < 11; n++)
				cksum += ((uint8_t*)p)[n];
			bHeaderChecksumOK = (uint8_t)(cksum - p->p_header_checksum) == p->p_header_checksum;
		}
		if (!bHeaderChecksumOK) {
			fprintf(stderr, "ROMConnection::OnEvent(): header checksum mismatch, giving up\n");
			return false;
		}

		if (p->p_flag & ROM_PACKET_FLAG_ALIVE_REQUEST) {
#if 0
			if (g_ROMProxy->GetDebugLevel() > 3)
				fprintf(stderr, "ROMConnection::OnEvent(): keepalive, responding\n");
#endif
			if (!SendKeepaliveReply())
				return false;
			m_Buffer->FlushData(p->p_length);
			continue;
		}

		if (p->p_flag != ROM_PACKET_FLAG_ENCRYPTED) {
			fprintf(stderr, "ROMConnection::OnEvent(): received un-encrypted packet, ignoring\n");
			m_Buffer->FlushData(p->p_length);
			continue;
		}

		if (!m_HaveKey) {
			fprintf(stderr, "ROMConnection::OnEvent(): encrypted packet while no key available, ignoring\n");
			m_Buffer->FlushData(p->p_length);
			continue;
		}

		// Obtain the key to use
		uint8_t key = p->p_keynum != 0xff ? m_Key[p->p_keynum] : 8;

		/* Decrypt (well, it's just plain mangling) */
		for (unsigned int n = 0; n < data_length; n++)
			p->p_data[n] = (p->p_data[n] + key) ^ key;

		/* Data checksum */
		bool bDataChecksumOK = true;
		{
			uint8_t cksum = 0;
			for (unsigned int n = 0; n < p->p_length; n++)
				cksum += ((uint8_t*)p)[n];
			cksum += key;
			cksum -= p->p_header_checksum;
			cksum -= p->p_data_checksum;
			bDataChecksumOK = cksum == p->p_data_checksum;
			if (!bDataChecksumOK)
				fprintf(stderr, "ROMConnection::OnEvent(): data checksum mismatch, expected %u got %u\n", cksum, p->p_data_checksum);
		}
		if (!bDataChecksumOK) {
			fprintf(stderr, "ROMConnection::OnEvent(): data checksum mismatch, giving up\n");
			return false;
		}

		// We have a valid packet. The callback object now decides what to do with it
		m_Callback.OnPacket(p);
		m_Buffer->FlushData(p->p_length);
	}

	return true;
}

bool
ROMConnection::SendKey(struct ROM::Packet* p, int len)
{
	assert(sizeof(struct ROM::Packet) + ROM_KEY_LENGTH <= len);

  /* Create the packet itself and off it goes */
  p->p_length = sizeof(struct ROM::Packet) + ROM_KEY_LENGTH;
  p->p_flag = ROM_PACKET_FLAG_KEY | ROM_PACKET_FLAG_ENCRYPTED;
  p->p_seq = 0;
  p->p_keynum = 0xff;
	// Copy the key; note that ChecksumAndSend() will use fixed key 8
	// since p_keynum == 0xff
	memcpy(&p->p_data[0], &m_Key[0], ROM_KEY_LENGTH);
	return ChecksumAndSend(p);
}

bool
ROMConnection::SendKeepaliveReply()
{
	struct ROM::Packet p;
	p.p_length = 0x10;
	p.p_flag = ROM_PACKET_FLAG_ALIVE_REPLY;
	p.p_seq = 0;
	p.p_keynum = 0xff;
	return ChecksumAndSend(&p);
}

bool
ROMConnection::SendPacket(struct ROM::Packet* p)
{
	assert(p->p_length >= sizeof(struct ROM::Packet));
	p->p_flag = ROM_PACKET_FLAG_ENCRYPTED;
	p->p_seq = m_Sequence;
	p->p_keynum = (m_Sequence - 1)  % 10;
	m_Sequence++;
	return ChecksumAndSend(p);
}

bool
ROMConnection::ChecksumAndSend(struct ROM::Packet* p)
{
	uint8_t key = (p->p_keynum != 0xff) ? m_Key[p->p_keynum] : 8;

	// Construct the header checksum value
	p->p_header_checksum = 0;
	p->p_data_checksum = 0;
	uint8_t hdr_cksum = 0;
	for (unsigned int n = 0; n < 11; n++)
		hdr_cksum += ((uint8_t*)p)[n];
	uint8_t data_cksum = 0;
	for (unsigned int n = 0; n < p->p_length; n++)
		data_cksum += ((uint8_t*)p)[n];

	// Data checksum requires the key to be added to it as wel
	data_cksum += key;

	// Store checksum values
	p->p_header_checksum = hdr_cksum + data_cksum;
	p->p_data_checksum = data_cksum;

	// Perform data encryption
	for (unsigned int n = 0; n < p->p_length - sizeof(struct ROM::Packet); n++)
		p->p_data[n] = (p->p_data[n] ^ key) - key;
	return m_Client.Write((const void*)p, p->p_length) == p->p_length;
}

void
ROMConnection::CopyKey(const ROMConnection& conn)
{
	assert(conn.m_HaveKey);
	memcpy(m_Key, conn.m_Key, ROM_KEY_LENGTH);
	m_HaveKey = true;
}

/* vim:set ts=2 sw=2: */
