/*
 * Runes of Magic protocol analysis - protocol analysis utility
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
#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "connection.h"
#include "csvsysparser.h"
#include "dataannotation.h"
#include "datatransformation.h"
#include "flow.h"
#include "protocoldefinition.h"
#include "romstate.h"
#include "romlogparser.h"
#include "tcpflowparser.h"
#include "types.h"
#include "../lib/romstructs.h"
#include "../lib/rompack.h"

#define LINE_MAX 256

IPv4Address server_addr;
IPv4Address client_addr;
uint8_t keys[ROM_KEY_LENGTH];

unsigned char hextab[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

#define PRINT printf

typedef std::map<Connection, Flow*> TConnectionFlowPtrMap;
typedef std::list<char*> TCharPtrList;

#define DISPLAY_SHOW_KEEPALIVE	1
#define DISPLAY_HEXDUMP 2
#define DISPLAY_KEY 4
#define SKIP_UNKNOWN 8

ROMState g_State;
ProtocolDefinition g_ProtocolDef;
TCharPtrList g_HideTypes;
TCharPtrList g_ShowTypes;
int g_DisplayFlags;
int g_IsROMLogFile;

class SysName : public XDataAnnotation {
public:
	bool Load(const char* fname);
	virtual const char* Lookup(uint32_t value);

private:
	CSVSysParser m_Parser;
};

SysName g_SysNames;

static char
resolve_addr(const IPv4Address& oAddress)
{
	if (oAddress == server_addr)
		return 's';
	if (oAddress == client_addr)
		return 'c';
	return '?';
}

static void
DumpData(const uint8_t* buf, int data_len)
{
	/* Dump */
	char output[LINE_MAX];
	output[255] = 0;
#define BYTES_PER_LINE 16
#define HEX_ASCII_SPACER 4
	memset(output, ' ', BYTES_PER_LINE * 3);
	for (int n = 0; n < HEX_ASCII_SPACER; n++)
		output[BYTES_PER_LINE * 3 + n] = ' ';
	int m = 0;
	// Enable for full packet dump including header
	for (unsigned int n = 0; n < data_len; n++) {
		uint8_t b = buf[n];

		/* Hex value */
		output[m * 3 + 0] = hextab[b >>  4];
		output[m * 3 + 1] = hextab[b & 0xf];

		/* ASCII, if printable */
		output[BYTES_PER_LINE * 3 + HEX_ASCII_SPACER + m] = isprint(b) ? b : '.';
		output[BYTES_PER_LINE * 3 + HEX_ASCII_SPACER + m + 1] = '\0';

		/* Display line if we need to */
		m++;
		if (m == BYTES_PER_LINE) {
			printf("%04x: %s\n", (n - m) + 1, output);
			m = 0;
		}
	}

	/* If we still have data left, show it */
	if (m > 0) {
		/* Clear fields that are not present */
		for (unsigned int n = m * 3; n < BYTES_PER_LINE * 3; n++) {
			output[n] = ' ';
		}
		printf("%04x: %s\n", data_len - m, output);
	}
}

static bool
PacketMatchesList(ProtocolDefinition::Packet& oPacket, const TCharPtrList& oList)
{
	for (auto it = oList.begin(); it != oList.end(); it++) {
		int packet_len, subpacket_len;
		const char* ptr = strchr(*it, ':');
		if (ptr == NULL) {
			packet_len = strlen(*it);
			subpacket_len = -1;
		} else {
			packet_len = ptr - *it;
			subpacket_len = strlen(*it) - packet_len - 1 /* : */;
		}

		if (strncmp(oPacket.GetName(), *it, packet_len) != 0)
			continue;

		if (subpacket_len > 0) {
			const ProtocolDefinition::Subpacket* pSubpacket = oPacket.GetSubpacket();
			if (pSubpacket == NULL)
				continue;

			if (strcmp(pSubpacket->GetName(), *it + packet_len + 1) != 0)
				continue;
		}

		// We have a match!
		return true;
	}

	return false;
}

static void
AnalyzePacket(Flow& oFlow, struct ROM::Packet* p, int sequence)
{
	// Skip keepalive packets unless instructed not to (they don't really give useful information)
	if ((p->p_flag & (ROM_PACKET_FLAG_ALIVE_REQUEST | ROM_PACKET_FLAG_ALIVE_REPLY)) != 0 /* keepalive */ &&
	    (g_DisplayFlags & DISPLAY_SHOW_KEEPALIVE) == 0)
		return;

	const Connection& oConn = oFlow.GetConnection();
	unsigned int data_len = p->p_length - sizeof(struct ROM::Packet);

	if (0) {
		char s[1024];
		sprintf(s, "dump%04u.bin", sequence);
		FILE* f = fopen(s, "wb");
		fwrite(p, p->p_length, 1, f);
		fclose(f);
	}
		
	if (p->p_flag & ROM_PACKET_FLAG_KEY) {
		if (data_len != ROM_KEY_LENGTH)
			errx(1, "key packet with wrong length (got %u, expected %u)", data_len, ROM_KEY_LENGTH);

		for (unsigned int n = 0; n < ROM_KEY_LENGTH; n++)
			g_State.m_Key[n] = (p->p_data[n] + 8) ^ 8;
		g_State.m_HaveKey = true;
		if ((g_DisplayFlags & DISPLAY_KEY) == 0)
			return; // nothing to see here
	}

	bool bHeaderChecksumOK = false;
	{
		/* Verify header checksum */
		uint8_t cksum = 0;
		for (unsigned int n = 0; n < 11; n++)
			cksum += ((uint8_t*)p)[n];
		bHeaderChecksumOK = (uint8_t)(cksum - p->p_header_checksum) == p->p_header_checksum;
	}

	bool bDataChecksumOK = true; // XXX we don't check in the unencrypted case
	if (p->p_flag == ROM_PACKET_FLAG_ENCRYPTED) {
		/* Fetch the key; it must have be known to us by now */
		uint8_t key = p->p_keynum != 0xff ? g_State.m_Key[p->p_keynum] : 8;

		if (!g_State.m_HaveKey) {
			PRINT(" [warning: no key available]");
		}

		/* Decrypt (well, it's just plain mangling) */
		for (unsigned int n = 0; n < data_len; n++)
			p->p_data[n] = (p->p_data[n] + key) ^ key;

		/* Data checksum */
		{
			uint8_t cksum = 0;
			for (unsigned int n = 0; n < p->p_length; n++)
				cksum += ((uint8_t*)p)[n];
			cksum += key;
			cksum -= p->p_header_checksum;
			cksum -= p->p_data_checksum;
			bDataChecksumOK = cksum == p->p_data_checksum;
		}
	}

	// See what the packet definitions make of it
	bool bSkipPacket = false;
	ProtocolDefinition::Packet* pPacket = NULL;
	if (p->p_flag == ROM_PACKET_FLAG_ENCRYPTED) {
		pPacket = g_ProtocolDef.Process(p->p_data, data_len);

		// If we need to skip this packet, do it
		if (pPacket != NULL) {
			bSkipPacket = PacketMatchesList(*pPacket, g_HideTypes);
			// If it isn't hidden, see if this is among the list we have to show - but
			// only if the list isn't empty (we'll show everything if it's empty)
			if (!bSkipPacket && !g_ShowTypes.empty() && !PacketMatchesList(*pPacket, g_ShowTypes))
				bSkipPacket = true;
		}
	}

	if (pPacket == NULL && (g_DisplayFlags & SKIP_UNKNOWN) != 0)
			bSkipPacket = true;

	if (bSkipPacket)
		return; // nothing to see here...

	PRINT(">>> %d: %s -> %s len %u flag 0x%x key %u seq %u",
	 sequence,
	 oConn.GetSource().ToString().c_str(),
	 oConn.GetDest().ToString().c_str(),
	 data_len,
	 p->p_flag, p->p_keynum, p->p_seq);

	if (!bHeaderChecksumOK)
		PRINT(" header checksum BAD");
	if (!bDataChecksumOK)
		PRINT(" data checksum BAD");
	PRINT("\n");

	// Enable below for dump with headers
#if 0
	const uint8_t* buf = (const uint8_t*)p;
	data_len = p->p_length;
#else
	const uint8_t* buf = (const uint8_t*)p->p_data;
#endif


	if (pPacket != NULL) {
		pPacket->Print(0);
		PRINT("\n");
	}

	if (pPacket == NULL || (g_DisplayFlags & DISPLAY_HEXDUMP)) {
		DumpData(buf, data_len);
	}

	PRINT("\n");
}

static void
AnalyzeFlow(Flow& oFlow, int& sequence)
{
	int iPrevDataLen = -1;
	const char* pData = oFlow.GetData() + oFlow.CurrentDataOffset();
	unsigned int iDataLeft = oFlow.GetDataLength() - oFlow.CurrentDataOffset();
	while(iDataLeft >= sizeof(struct ROM::Packet)) {
		struct ROM::Packet* p = (struct ROM::Packet*)pData;
		if (p->p_length > 131072) {
				// XXX Figure out the exact value
				printf("AnalyzeFlow(): obscenely large packet length %u, aborting\n", p->p_length);
				abort();
		}

		if (p->p_length < sizeof(*p) || p->p_length > iDataLeft)
			break; // not enough bytes of this packet to process

		AnalyzePacket(oFlow, p, sequence);
		pData += p->p_length;
		iPrevDataLen = p->p_length;
		oFlow.CurrentDataOffset() += p->p_length;
		iDataLeft -= p->p_length;

		sequence++;
	}
}

class ROMPacking : public XDataTransformation {
public:
	virtual bool Apply(const uint8_t* pSource, int iSourceLen, uint8_t* pDest, int& oDestLen);
	virtual int EstimateBufferSize(const uint8_t* pSource, int iSourceLen);
};

bool
ROMPacking::Apply(const uint8_t* pSource, int iSourceLen, uint8_t* pDest, int& oDestLen)
{
	if (iSourceLen == 0)
		return true; // nothing to do

	// For some reason, some packets are packed with an extra xx xx xx xx 11 10
	// 00 00 trailer. We must discard it from the input. No idea why this is done
	if (iSourceLen > 8 &&
			pSource[iSourceLen - 4] == 0x11 &&
			pSource[iSourceLen - 3] == 0x10 &&
			pSource[iSourceLen - 2] == 0x00 &&
			pSource[iSourceLen - 1] == 0x00) {
		printf("wonky\n");
		iSourceLen -= 8;
	}

	int iOutLen;
	if (!ROMPack::Unpack(pSource, iSourceLen, pDest, &iOutLen)) {
		printf("ROMPACK UNPACK FAILURE!!!\n");
		return false;
	}

	// XXX very useful when debugging; maybe this should become a flag...
#if 0
	static int unpack_no = 0;
	char fname[255];
	sprintf(fname, "/tmp/unpack%03d.bin", unpack_no++);
	FILE* f = fopen(fname, "wb");
	fwrite(pDest, iOutLen, 1, f);
	fclose(f);

	printf(">> unpacked written to '%s'\n", fname);
#endif

	oDestLen = iOutLen;
	return true;
}

int
ROMPacking::EstimateBufferSize(const uint8_t* pSource, int iSourceLen)
{
	return 1024 * 1024; // XXX crude, but never saw something that large
}

static void
usage(const char* progname)
{	
	fprintf(stderr, "usage: %s [-hkuxyo?] [-d protocol.xml] [-i filter] [-j filter] [-s sysfile.csv] file.txt\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "  -h, -?             this help\n");
	fprintf(stderr, "  -d protocol.xml    use supplied protocol definitions\n");
	fprintf(stderr, "  -k                 display keepalive request/replies\n");
	fprintf(stderr, "  -o                 print offsets of fields within packets\n");
	fprintf(stderr, "  -x                 always display hexdump of packet\n");
	fprintf(stderr, "                     (default: only if no definition available)\n");
	fprintf(stderr, "  -y                 display key exchange packets\n");
	fprintf(stderr, "  -i [filter]        ignore packets matching [filter]\n");
	fprintf(stderr, "  -j [filter]        only accept packets matching [filter]\n");
	fprintf(stderr, "  -s sysfile.csv     use Sys_... ID definitions\n");
	fprintf(stderr, "  -u                 ignore unrecognized packets\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "filter are comma-separated and match by packet type. A subpacket can be matched by using 'packet:subpacket'\n");
}

static void
parse_list(char* arg, TCharPtrList& oList)
{
	char* cur = optarg;
	while(1) {
		char* ptr = strchr(cur, ',');
		if (ptr != NULL)
			*ptr++ = '\0';
		oList.push_back(strdup(cur));
		if (ptr == NULL)
			break;
		cur = ptr;
	}
}

bool
SysName::Load(const char* fname)
{
	return m_Parser.Load(fname);
}

const char*
SysName::Lookup(uint32_t value)
{
	return m_Parser.Lookup(value).c_str(); // safe because const string&
}

class StatName : public XDataAnnotation {
public:
	virtual const char* Lookup(uint32_t value);
};

const char*
StatName::Lookup(uint32_t value)
{
	// stat names are stored as a 16-bit value; they just need a delta value added
	return g_SysNames.Lookup(500000 + value);
}

static ProtocolDefinition::Field*
FindFieldByName(ProtocolDefinition::Struct* pStruct, const char* name)
{
	auto actions = pStruct->GetActions();
	for (auto it = actions.begin(); it != actions.end(); it++) {
		ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(*it);
		if (pField == NULL)
			continue;

		if (strcmp(pField->GetName(), name) == 0)
			return pField;
	}

	return NULL;
}

class ObjectIdStore : public XDataAnnotation
{
public:
	virtual const char* Lookup(uint32_t value);

	virtual void Apply(ProtocolDefinition::Type* pType);

protected:
	typedef std::map<int, std::string> TintStringMap;
	TintStringMap m_Ids;
};

const char*
ObjectIdStore::Lookup(uint32_t value)
{
	auto it = m_Ids.find(value);
	if (it == m_Ids.end())
		return "?";
	return it->second.c_str();
}

void
ObjectIdStore::Apply(ProtocolDefinition::Type* pType)
{
	/*
	 * XXX This entire function is an entire kludge which should be made without all the casts and
	 *     diving into pType's internals...
	 */
	ProtocolDefinition::Struct* pStruct = dynamic_cast<ProtocolDefinition::Struct*>(pType);

	auto pCharIdField = FindFieldByName(pStruct, "objectid");
	auto pRaceIdField = FindFieldByName(pStruct, "race");
	auto pNameIdField = FindFieldByName(pStruct, "name");
	if (pCharIdField == NULL || pRaceIdField == NULL || pNameIdField == NULL) {
		fprintf(stderr, "warning: applying 'objectid' annotation without required fields objectid/raceid/name, skipping\n");
		return;
	}

	auto pCharIdValue = dynamic_cast<ProtocolDefinition::unsignedType*>(&pCharIdField->GetType());
	auto pNameValue = dynamic_cast<ProtocolDefinition::stringType*>(&pNameIdField->GetType());
	if (pCharIdValue == NULL || pNameValue == NULL) {
		fprintf(stderr, "warning: applying 'objectid' annotation with incorrect field types, skipping\n");
		return;
	}
	unsigned int objectid = pCharIdValue->GetValue(0);

	char value[256];
	if (pNameValue->GetValue()[0] != '\0') {
		strncpy(value, pNameValue->GetValue(), sizeof(value) - 1);
		value[sizeof(value) - 1] = '\0';
	} else {
		pRaceIdField->GetType().GetHumanReadableContent(value, sizeof(value));

		// XXX Kludge away the <id> thing if it exists
		char* ptr = strrchr(value, '<');
		if (ptr != NULL) {
			*ptr = '\0';

			// And remove trailing spaces, too ..
			int n = strlen(value);
			while (n > 0 && value[n - 1] == ' ')
				n--;
			value[n] = '\0';
		}
	}

	// We got it, we can store it
	m_Ids.insert(std::pair<int, std::string>(objectid, value));
}

class CharId2ObjectIdAnnotation : public XDataAnnotation
{
public:
	CharId2ObjectIdAnnotation(ObjectIdStore& obj_store) : m_ObjectStore(obj_store)
	{
	}
	virtual const char* Lookup(uint32_t value);

	virtual void Apply(ProtocolDefinition::Type* pType);

protected:
	typedef std::map<int, int> TintStringMap;
	TintStringMap m_Ids;
	ObjectIdStore& m_ObjectStore;
};

const char*
CharId2ObjectIdAnnotation::Lookup(uint32_t value)
{
	auto it = m_Ids.find(value);
	if (it == m_Ids.end())
		return "?";
	return m_ObjectStore.Lookup(it->second);
}

void
CharId2ObjectIdAnnotation::Apply(ProtocolDefinition::Type* pType)
{
	/*
	 * XXX This entire function is an entire kludge which should be made without all the casts and
	 *     diving into pType's internals...
	 */
	ProtocolDefinition::Struct* pStruct = dynamic_cast<ProtocolDefinition::Struct*>(pType);

	auto pCharIdField = FindFieldByName(pStruct, "charid");
	auto pObjectIdField = FindFieldByName(pStruct, "objectid");
	if (pCharIdField == NULL || pObjectIdField == NULL) {
		fprintf(stderr, "warning: applying 'charid' annotation without required field objectid, skipping\n");
		return;
	}

	auto pCharIdValue = dynamic_cast<ProtocolDefinition::unsignedType*>(&pCharIdField->GetType());
	auto pObjectIdValue = dynamic_cast<ProtocolDefinition::unsignedType*>(&pObjectIdField->GetType());
	if (pCharIdValue == NULL || pObjectIdField == NULL) {
		fprintf(stderr, "warning: applying 'charid' annotation with incorrect field types, skipping\n");
		return;
	}
	unsigned int charid = pCharIdValue->GetValue(0);
	unsigned int objectid = pObjectIdValue->GetValue(0);

	// We got it, we can store it
	m_Ids.insert(std::pair<int, int>(charid, objectid));
}

int
main(int argc, char** argv)
{
	g_ProtocolDef.RegisterTransformation("rompack", *new ROMPacking);
	g_ProtocolDef.RegisterAnnotation("sys_name", g_SysNames);
	g_ProtocolDef.RegisterAnnotation("stat_name", *new StatName);
	ObjectIdStore* pObjectStore = new ObjectIdStore;
	g_ProtocolDef.RegisterAnnotation("objectid", *pObjectStore);
	g_ProtocolDef.RegisterAnnotation("charid", *new CharId2ObjectIdAnnotation(*pObjectStore));

	{
		int opt;
		while ((opt = getopt(argc, argv, "?hd:i:j:ks:uxyo")) != -1) {
			switch(opt) {
				case 'd':
					if (!g_ProtocolDef.Load(optarg))
						errx(1, "can't load protocol definitions");
					break;
				case 'k':
					g_DisplayFlags |= DISPLAY_SHOW_KEEPALIVE;
					break;
				case 'i':
					parse_list(optarg, g_HideTypes);
					break;
				case 'j':
					parse_list(optarg, g_ShowTypes);
					break;
				case 'u':
					g_DisplayFlags |= SKIP_UNKNOWN;
					break;
				case 'x':
					g_DisplayFlags |= DISPLAY_HEXDUMP;
					break;
				case 'y':
					g_DisplayFlags |= DISPLAY_KEY;
					break;
				case 's':
					if (!g_SysNames.Load(optarg))
						errx(1, "can't load sys names");
					break;
				case 'o':
					g_ProtocolDef.SetPrintDataOffset(true);
					break;
				case 'h':
				case '?':
				default:
					usage(argv[0]);
					return EXIT_FAILURE;
			}
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "missing file to process\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	FILE* f = fopen(argv[optind], "rb");
	if (f == NULL)
		err(1, "can't open '%s'", argv[optind]);

	// See if this is a ROM binary log file
	{
		struct ROM::LoggerHeader lh;
		g_IsROMLogFile = 0;
		if (fread(&lh, sizeof(lh), 1, f)) {
			if (lh.lh_magic == ROM_LOGGER_HEADER_MAGIC_1)
				g_IsROMLogFile = 1;
			else if (lh.lh_magic == ROM_LOGGER_HEADER_MAGIC_2)
				g_IsROMLogFile = 2;
		} else {
			// Not a ROM binary log file
			rewind(f);
		}
	}

	TConnectionFlowPtrMap flows;
	
	int sequence = 1;
	char pBuffer[131072];
	bool bOK = true;
	while(bOK) {
		IPv4Address oSource, oDest;
		int iLength = 0;
		if (g_IsROMLogFile) {
			// ROM binary log file
			int iResult = ROMLogParser::ParseHeader(f, oSource, oDest, g_IsROMLogFile >= 2);
			bOK = iResult >= 0;
			if (!bOK || iResult == 0)
				break;
			if (iResult <= sizeof(pBuffer)) {
				iLength = ROMLogParser::ReadPacket(f, pBuffer, iResult);
			} else {
				PRINT("excessive packet size %u, aborting\n", iResult);
				iLength = 0;
			}
		} else {
			// TCPFlow file
			int iResult = TCPFlowParser::ParseHeader(f, oSource, oDest);
			bOK = iResult >= 0;
			if (!bOK || iResult == 0)
				break;
			iLength = TCPFlowParser::ParsePacket(f, pBuffer, sizeof(pBuffer));
		}
		bOK = iLength >= 0;
		if (!bOK || iLength == 0)
			break;

		std::pair<TConnectionFlowPtrMap::iterator, bool> oResult = flows.insert(std::pair<Connection, Flow*>(Connection(oSource, oDest), NULL));
		if (oResult.second) {
			// New element; need to hook it up (done here to prevent memory leak)
			Flow* pFlow = new Flow(oResult.first->first, 262000 /* XXX */);
			oResult.first->second = pFlow;
		}

		Flow* pFlow = oResult.first->second;
		pFlow->Append(pBuffer, iLength);

		/*
		 * Process as much as we can of the stream; we want to match
		 * request/response as much as possible. This means we will interleave
		 * the output, so we have to print the source/destination address
		 * as well.
		 */
		AnalyzeFlow(*pFlow, sequence);
	}

	// Walk through the flows and see if they are completed; while here, clean 'm up!
	for (TConnectionFlowPtrMap::iterator it = flows.begin(); it != flows.end(); it++) {
		Flow* pFlow = it->second;
		if (pFlow->CurrentDataOffset() != pFlow->GetDataLength()) {
			const Connection& oConn = pFlow->GetConnection();
			int iLeft = pFlow->GetDataLength() - pFlow->CurrentDataOffset();
			PRINT("WARNING: %s -> %s still has %u bytes of unprocessed data left!\n",
			 oConn.GetSource().ToString().c_str(),
			 oConn.GetDest().ToString().c_str(),
			 iLeft);
			 
			// See if we can isolate  the header
			if (iLeft >= sizeof(struct ROM::Packet)) {
				const char* pData = pFlow->GetData() + pFlow->CurrentDataOffset();
				struct ROM::Packet* p = (struct ROM::Packet*)pData;
				PRINT("first flow packet length: %u bytes\n", p->p_length);
			}
		}
		delete pFlow;
	}

	return EXIT_SUCCESS;
}

/* vim:set ts=2 sw=2: */
