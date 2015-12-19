/*
 * Runes of Magic protocol anaylsis - code generator
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
#include "protocolcodegenerator.h"
#include <stdlib.h>
#include <string.h>
#include "protocoldefinition.h"

ProtocolCodeGenerator::ProtocolCodeGenerator(ProtocolDefinition& oDef)
	: m_Definition(oDef)
{
}

void
ProtocolCodeGenerator::GenerateEnumerations(FILE* f)
{
	auto oEnums = m_Definition.GetEnumerations();
	for (auto& oEnum: oEnums) {
		fprintf(f, "enum enum_%s {\n", oEnum->GetName());
		for (auto& oKeyValue: oEnum->GetValueMap()) {
			fprintf(f, "\tE_%s = %u,\n",
		 	 oKeyValue.second, oKeyValue.first);
		}
		fprintf(f, "};\n");
		fprintf(f, "\n");
	}
}

void
ProtocolCodeGenerator::GenerateTypes(FILE* f)
{
	char sType[ProtocolDefinition::s_GenerateMaxLength];
	char sSuffix[ProtocolDefinition::s_GenerateMaxLength];

	auto oTypes = m_Definition.GetTypes();
	for (auto& oType: oTypes) {
		// XXX Skip all non-structs for now
		ProtocolDefinition::Struct* pStruct = dynamic_cast<ProtocolDefinition::Struct*>(oType);
		if (pStruct == NULL)
			continue;
		fprintf(f, "struct %s {\n", pStruct->GetName());
		for (auto& oAction: pStruct->GetActions()) {
			ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
			if (pField == NULL)
				continue; // XXX skips transformations

			pField->GetType().GenerateCType(sType, sSuffix);
			fprintf(f, "\t%s %s%s;\n", sType, pField->GetName(), sSuffix);
		}
		fprintf(f, "} PACKED;\n");
		fprintf(f, "\n");
	}
}

void
ProtocolCodeGenerator::GenerateInitializers(FILE* f, const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sPrefix)
{
	for (auto& oAction: oActions) {
		ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
		if (pField == NULL)
			continue; // XXX skips transformations
		char sInit[1024];
		pField->GetType().GenerateCInitialize(sInit, sizeof(sInit));
		if (sInit[0] == '\0')
			continue;
		fprintf(f, "\tm_Packet.%s%s = %s;\n", sPrefix, pField->GetName(), sInit);
	}
}

void
ProtocolCodeGenerator::GenerateChangers(FILE* f, const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sPrefix, const char* sFuncPrefix)
{
	char sType[ProtocolDefinition::s_GenerateMaxLength];
	char sSuffix[ProtocolDefinition::s_GenerateMaxLength];

	for (auto& oAction: oActions) {
		ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
		if (pField == NULL)
			continue; // XXX skips transformations
		pField->GetType().GenerateCType(sType, sSuffix);
		if (sSuffix[0] != '\0') {
			fprintf(f, "\t%s* %s%s() { return m_Packet.%s%s; }\n",
			 sType,
			 sFuncPrefix,
			 pField->GetName(),
			 sPrefix,
			 pField->GetName());
		} else {
			fprintf(f, "\tvoid %s%s(%s n) { m_Packet.%s%s = n; }\n",
			 sFuncPrefix,
			 pField->GetName(),
			 sType,
			 sPrefix,
			 pField->GetName());
			fprintf(f, "\t%s %s%s() const { return m_Packet.%s%s; }\n",
			 sType,
			 sFuncPrefix,
			 pField->GetName(),
			 sPrefix,
			 pField->GetName());
		}
	}
}

void
ProtocolCodeGenerator::GenerateFields(FILE* f, const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sPrefix)
{
	char sType[ProtocolDefinition::s_GenerateMaxLength];
	char sSuffix[ProtocolDefinition::s_GenerateMaxLength];

	for (auto& oAction: oActions) {
		ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
		if (pField == NULL)
			continue; // XXX skips transformations
		pField->GetType().GenerateCType(sType, sSuffix);
		fprintf(f, "\t\t%s %s%s%s;\n", sType, sPrefix, pField->GetName(), sSuffix);
	}
}

void
ProtocolCodeGenerator::GeneratePackets(FILE* f)
{

	auto oPackets = m_Definition.GetPacketTypes();
	for (auto& oPacket: oPackets) {
		fprintf(f, "class %s {\n", oPacket->GetName());
		fprintf(f, "\tfriend class Parser;\n");
		fprintf(f, "public:\n");
		fprintf(f, "\t%s() { Reset(); }\n", oPacket->GetName());
		fprintf(f, "\tvoid Reset();\n", oPacket->GetName());
		// Generate default values for all fields
		fprintf(f, "\tvoid Send(State& oStater, int iDelta = 0);\n");
		fprintf(f, "\n");

		// Generator functions to change all values
		GenerateChangers(f, oPacket->GetActions(), "m_", "");
		fprintf(f, "\n");

		fprintf(f, "protected:\n");
		fprintf(f, "\t%s(const char* pData, int iDataLength);\n", oPacket->GetName());
		fprintf(f, "\n");
		fprintf(f, "private:\n");
		fprintf(f, "\tstruct Packet {\n");
		fprintf(f, "\t\tstruct ROM::Packet m_header;\n");
		GenerateFields(f, oPacket->GetActions(), "m_");
		fprintf(f, "\t} PACKED;\n");
		fprintf(f, "\tPacket m_Packet;\n");
		fprintf(f, "};\n");

		// Subpackets
		for (auto& oSubpacket: oPacket->GetSubpackets()) {
			fprintf(f, "class %s_%s {\n", oPacket->GetName(), oSubpacket->GetName());
			fprintf(f, "\tfriend class Parser;\n");
			fprintf(f, "public:\n");

			// Constructor with default values for all fields
			fprintf(f, "\t%s_%s() { Reset(); }\n", oPacket->GetName(), oSubpacket->GetName());
			fprintf(f, "\tvoid Reset();\n");
			bool bPackerKludge = false;
			if (true /* oPacket->GetSource() == ProtocolDefinition::Packet::S_Server */) {
				fprintf(f, "\tvoid Send(State& oState");

				// See if we have transformations; we need to generate a different
				// prototype in such a case
				const ProtocolDefinition::Struct::TXActionPtrList& oActions = oSubpacket->GetActions();
				ProtocolDefinition::Struct::TXActionPtrList::const_iterator oTransformAction = oActions.end();
				for (auto it = oActions.begin(); it != oActions.end(); it++) {
					if (dynamic_cast<ProtocolDefinition::TransformationAction*>(*it) == NULL)
						continue;
					// Okay, we found a transformation
					oTransformAction = it;
					break;
				}

				if (oTransformAction == oActions.end()) {
					fprintf(f, ", int iDelta = 0");
				} else {
					// Construct the fields to send
					for (auto it = oTransformAction; it != oActions.end(); it++) {
						ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(*it);
						if (pField == NULL)
							continue;
						fprintf(f, ", const %s& %s", pField->GetType().GetName(), pField->GetName());
					}
					bPackerKludge = true;
				}
				fprintf(f, ");\n");
			}
			fprintf(f, "\n");

			// Generator functions to change all values
			GenerateChangers(f, oPacket->GetActions(), "m_packet_", "packet_");
			fprintf(f, "\n");
			GenerateChangers(f, oSubpacket->GetActions(), "m_", "");
			fprintf(f, "\n");

			if (true /* oPacket->GetSource() == ProtocolDefinition::Packet::S_Client */) {
				fprintf(f, "protected:\n");
				fprintf(f, "\t%s_%s(const char* pData, int iDataLength);\n", oPacket->GetName(), oSubpacket->GetName());
				fprintf(f, "\n");
			}

			//
			fprintf(f, "private:\n");
			fprintf(f, "\tstruct Packet {\n");
			fprintf(f, "\t\tstruct ROM::Packet m_packet_header;\n");
			fprintf(f, "\t\t// %s\n", oPacket->GetName());
			GenerateFields(f, oPacket->GetActions(), "m_packet_");
			fprintf(f, "\t\t// %s\n", oSubpacket->GetName(), "m_");
			GenerateFields(f, oSubpacket->GetActions(), "m_");

			// XXX This is a horrible kludge to add some space for the packer...
			if (bPackerKludge)
				fprintf(f, "\t\tuint8_t __packer_kludge[1024];\n");
			fprintf(f, "\t} PACKED;\n");

			// Data member
			fprintf(f, "\tPacket m_Packet;\n");
			fprintf(f, "};\n");
			fprintf(f, "\n");
		}
		fprintf(f, "\n");
	}
}

uint32_t
ProtocolCodeGenerator::GenerateLength(FILE* f, ProtocolDefinition::Packet& oPacket, ProtocolDefinition::Subpacket& oSubpacket, const char* sPrefix)
{
	// Determine subpacket length
	uint32_t iSubpacketLen = 0;
	for (auto& oAction: oSubpacket.GetActions()) {
		ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
		if (pField == NULL)
			continue;
		iSubpacketLen += pField->GetType().GetConstantSize();
	}

	// XXX We'll just fill any length field in the main packet...
	for (auto& oAction: oPacket.GetActions()) {
		ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
		if (pField == NULL || strcmp(pField->GetType().GetName(), "length") != 0)
			continue;
		fprintf(f, "\t%s%s = 0x%x;\n", sPrefix, pField->GetName(), iSubpacketLen);
	}
	return iSubpacketLen;
}

void
ProtocolCodeGenerator::GenerateFunctions(FILE* f)
{
	auto oPackets = m_Definition.GetPacketTypes();
	for (auto& oPacket: oPackets) {
		fprintf(f, "void\n");
		fprintf(f, "%s::Reset()\n", oPacket->GetName());
		fprintf(f, "{\n");
		fprintf(f, "\tmemset((void*)&m_Packet, 0, sizeof(m_Packet));\n");
		GenerateInitializers(f, oPacket->GetActions(), "m_");
		fprintf(f, "}\n");
		fprintf(f, "\n");


		// Figure out the length of the packet itself
		uint32_t iPacketLength = 0;
		for (auto& oAction: oPacket->GetActions()) {
			ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
			if (pField == NULL)
				continue;
			iPacketLength += pField->GetType().GetConstantSize();
		}

		if (true /* oPacket->GetSource() == ProtocolDefinition::Packet::S_Server */) {
			fprintf(f, "void\n");
			fprintf(f, "%s::Send(State& oState, int iDelta /* = 0 */)\n", oPacket->GetName());
			fprintf(f, "{\n");
			fprintf(f, "\toState.Send((struct ROM::Packet&)m_Packet, sizeof(struct ROM::Packet) + 0x%x + iDelta);\n", iPacketLength);
			fprintf(f, "}\n");
			fprintf(f, "\n");
		}
		if (true /* oPacket->GetSource() == ProtocolDefinition::Packet::S_Client */) {
			fprintf(f, "%s::%s(const char* pData, int iDataLength)\n", oPacket->GetName(), oPacket->GetName());
			fprintf(f, "{\n");
			fprintf(f, "\tassert(iDataLength <= (int)sizeof(m_Packet));\n");
			fprintf(f, "\tmemcpy((void*)&m_Packet, pData, sizeof(m_Packet));\n");
			fprintf(f, "}\n");
			fprintf(f, "\n");
		}

		// XXX Find a length field
		ProtocolDefinition::Field* pLengthField = NULL;
		for (auto& oAction: oPacket->GetActions()) {
			ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
			if (pField == NULL || strcmp(pField->GetType().GetName(), "length") != 0)
				continue;
			pLengthField = pField;
			break;
		}

		// Subpackets
		for (auto& oSubpacket: oPacket->GetSubpackets()) {
			fprintf(f, "void\n");
			fprintf(f, "%s_%s::Reset()\n", oPacket->GetName(), oSubpacket->GetName());
			fprintf(f, "{\n");
			fprintf(f, "\tmemset((void*)&m_Packet, 0, sizeof(m_Packet));\n");
			GenerateInitializers(f, oPacket->GetActions(), "m_packet_");
			GenerateInitializers(f, oSubpacket->GetActions(), "m_");
			uint32_t iSubLen = GenerateLength(f, *oPacket, *oSubpacket, "m_Packet.m_packet_");
			fprintf(f, "}\n");
			fprintf(f, "\n");

			if (true /* oPacket->GetSource() == ProtocolDefinition::Packet::S_Server */) {
				/*
				 * In the server -> client case, there may be non-trivial
				 * transformations lurking: compression, encryption and the like. For
				 * now, we'll special-case these by taking all fields as arguments and
				 * applying the transformation here and now.
				 *
				 * XXX This may be considered a bit specific, but it's the best we can
				 * do for now...
				 */
				const ProtocolDefinition::Struct::TXActionPtrList& oActions = oSubpacket->GetActions();
				ProtocolDefinition::Struct::TXActionPtrList::const_iterator oTransformAction = oActions.end();
				for (auto it = oActions.begin(); it != oActions.end(); it++) {
					if (dynamic_cast<ProtocolDefinition::TransformationAction*>(*it) == NULL)
						continue;
					// Okay, we found a transformation
					oTransformAction = it;
					break;
				}

				if (oTransformAction == oActions.end()) {
					fprintf(f, "void\n");
					fprintf(f, "%s_%s::Send(State& oState, int iDelta /* = 0 */)\n", oPacket->GetName(), oSubpacket->GetName());
					fprintf(f, "{\n");
					if (pLengthField != NULL) {
						// XXX This is a kludge
						fprintf(f, "\tm_Packet.m_packet_%s += iDelta; // KLUDGE\n", pLengthField->GetName());
					}
					fprintf(f, "\toState.Send((struct ROM::Packet&)m_Packet, sizeof(struct ROM::Packet) + 0x%x + 0x%x + iDelta);\n", iPacketLength, iSubLen);
					fprintf(f, "}\n");
					fprintf(f, "\n");
				} else {
					// Transformations are here; need to generate the new Send() version
					ProtocolDefinition::TransformationAction* pTransformation = dynamic_cast<ProtocolDefinition::TransformationAction*>(*oTransformAction);

					fprintf(f, "void\n");
					fprintf(f, "%s_%s::Send(State& oState", oPacket->GetName(), oSubpacket->GetName());

					// Generate the argument list
					for (auto it = oTransformAction; it != oActions.end(); it++) {
						ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(*it);
						if (pField == NULL)
							continue;
						fprintf(f, ", const %s& %s", pField->GetType().GetName(), pField->GetName());
					}
					fprintf(f, ")\n");
					fprintf(f, "{\n");

					// Look for a length field before the transformations; we'll need to fill it out as well
					ProtocolDefinition::Field* pTransformedLengthField = NULL;
					for (auto it = oActions.begin(); it != oTransformAction; it++) {
						ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(*it);
						if (pField == NULL || strcmp(pField->GetType().GetName(), "length") != 0)
							continue;
						pTransformedLengthField = pField;
						break;
					}

					// Transform all fields directly to the destination; this assumes
					// things will not get larger XXX We work around this by appending
					// dummy data at the end of the struct
					fprintf(f, "\tint iCurrentLength = 0;\n");
					fprintf(f, "\tint iPreviousLength = 0;\n");
					for (auto it = oTransformAction; it != oActions.end(); it++) {
						ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(*it);
						if (pField == NULL)
							continue;

						fprintf(f, "\t{\n");
						fprintf(f, "\t\tint iOutLen = 1024 * 1024; // XXX;\n");
						fprintf(f, "\t\tif (!%s::Pack((const uint8_t*)&%s, sizeof(%s), (uint8_t*)&m_Packet.m_%s, &iOutLen))\n",
						 pTransformation->GetTransformation().GetName(), pField->GetName(), pField->GetName(), pField->GetName());
						fprintf(f, "\t\t\tassert(0);\n");
						fprintf(f, "\n");
						fprintf(f, "\t\tiPreviousLength += sizeof(%s);\n", pField->GetName());
						fprintf(f, "\t\tiCurrentLength += iOutLen;\n");
						fprintf(f, "\t\tm_Packet.m_packet_%s = m_Packet.m_packet_%s - sizeof(%s) + iOutLen;\n",
						 pLengthField->GetName(), pLengthField->GetName(), pField->GetName());
						fprintf(f, "\t}\n");
						fprintf(f, "\n");
					}

					// Set the length field, if any
					if (pTransformedLengthField != NULL) {
						fprintf(f, "\tm_Packet.m_%s = iCurrentLength;\n", pTransformedLengthField->GetName());
					}

					// XXX The send is kludgy; the x - iPreviousLength can be avoided...
					fprintf(f, "\toState.Send((struct ROM::Packet&)m_Packet, sizeof(struct ROM::Packet) + 0x%x + 0x%x - iPreviousLength + iCurrentLength);\n", iPacketLength, iSubLen);
					fprintf(f, "}\n");
					fprintf(f, "\n");
				}
			}
			if (true /* oPacket->GetSource() == ProtocolDefinition::Packet::S_Client */) {
				// Check for any transformations
				const ProtocolDefinition::Struct::TXActionPtrList& oActions = oSubpacket->GetActions();
				ProtocolDefinition::Struct::TXActionPtrList::const_iterator oTransformAction = oActions.end();
				for (auto it = oActions.begin(); it != oActions.end(); it++) {
					if (dynamic_cast<ProtocolDefinition::TransformationAction*>(*it) == NULL)
						continue;
					// Okay, we found a transformation
					oTransformAction = it;
					break;
				}

				fprintf(f, "%s_%s::%s_%s(const char* pData, int iDataLength)\n",
				 oPacket->GetName(), oSubpacket->GetName(), oPacket->GetName(), oSubpacket->GetName());
				fprintf(f, "{\n");
				fprintf(f, "\tassert(iDataLength <= (int)sizeof(m_Packet));\n");
				if (oTransformAction != oActions.end()) {
					/*
					 * There are transformations here; first, copy what we have in front
					 * of the transformed data.
					 */
					uint32_t iNonTransformedLength = 0;
					for (auto it = oActions.begin(); it != oTransformAction; it++) {
						ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(*it);
						if (pField == NULL)
							continue;
						iNonTransformedLength += pField->GetType().GetConstantSize();
					}
					fprintf(f, "\tint iTransformedOffset = sizeof(ROM::Packet) + 0x%x + 0x%x;\n", iPacketLength, iNonTransformedLength);
					fprintf(f, "\tmemcpy((void*)&m_Packet, pData, iTransformedOffset);\n");

					/* Now we'll have to transform whatever is left */
					ProtocolDefinition::TransformationAction* pTransformation = dynamic_cast<ProtocolDefinition::TransformationAction*>(*oTransformAction);
					for (auto it = oTransformAction; it != oActions.end(); it++) {
						ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(*it);
						if (pField == NULL)
							continue;
						fprintf(f, "\t{\n");
						fprintf(f, "\t\tint iOutLen = sizeof(%s);\n", pField->GetType().GetName());
						fprintf(f, "\t\tif (!%s::Unpack((const uint8_t*)&pData[iTransformedOffset], iDataLength - iTransformedOffset, (uint8_t*)&m_Packet.m_%s, &iOutLen))\n",
						 pTransformation->GetTransformation().GetName(), pField->GetName());
						fprintf(f, "\t\t\tfprintf(stderr, \"WARNING: unable to decode packing for '%s'!\\n\");\n", oSubpacket->GetName());
						fprintf(f, "\t}\n");
						fprintf(f, "\n");
						// XXX We assume there will not be a second packed field after here
					}
				} else /* oTransformAction == oActions.end() */ {
					fprintf(f, "\tmemcpy((void*)&m_Packet, pData, sizeof(m_Packet));\n");
				}
				fprintf(f, "}\n");
				fprintf(f, "\n");
			}
		}
	}
}

bool
ProtocolCodeGenerator::GenerateMatchCondition(const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sPacketName, char* sOutput, int iOutputLen)
{
	int iOffset = 0;
	for (auto& oAction: oActions) {
		ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
		if (pField == NULL)
			continue; // XXX skips transformations
		char sInit[1024];
		pField->GetType().GenerateCInitialize(sInit, sizeof(sInit));
		if (sInit[0] == '\0')
			continue;
		if (iOffset > 0)
			iOffset += snprintf(sOutput + iOffset, iOutputLen - iOffset, " && ");
		iOffset +=
		 snprintf(sOutput + iOffset, iOutputLen - iOffset, "((struct %s::Packet*)pData)->m_%s == %s",
			sPacketName, pField->GetName(), sInit);
	}

	return iOffset > 0;
}

void
ProtocolCodeGenerator::GenerateParser(FILE* f, const char* sPacketPrefix)
{
	fprintf(f, "bool\n");
	fprintf(f, "Parser::Process(char* pData, int iDataLength)\n");
	fprintf(f, "{\n");

	// Standard length checks
	fprintf(f, "\tif (iDataLength < (int)sizeof(ROM::Packet))\n");
	fprintf(f, "\t\treturn false;\n");
	fprintf(f, "\tif (((struct ROM::Packet*)pData)->p_length != iDataLength)\n");
	fprintf(f, "\t\treturn false;\n");

	auto oPackets = m_Definition.GetPacketTypes();
	for (auto& oPacket: oPackets) {
#if 0
		// Skip anything not originating from the client
		if (oPacket->GetSource() != ProtocolDefinition::Packet::S_Client)
			continue;
#endif

		// Walk through all fields, creating the matching criterium
		char sCriterium[1024];
		if (!GenerateMatchCondition(oPacket->GetActions(), oPacket->GetName(), sCriterium, sizeof(sCriterium)))
			continue; // no fixed values

		/* Find the length field; we'll use it to ensure the packet is okay */
		ProtocolDefinition::Field* pLengthField = NULL;
		for (auto& oAction: oPacket->GetActions()) {
			ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
			if (pField == NULL || strcmp(pField->GetType().GetName(), "length") != 0)
				continue;
			pLengthField = pField;
			break;
		}

		// Determine packet length
		uint32_t iPacketLen = 0;
		for (auto& oAction: oPacket->GetActions()) {
			ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
			if (pField == NULL)
				continue;
			iPacketLen += pField->GetType().GetConstantSize();
		}

		// Generate match conditions; this will also check the lengths, if we can do so
		fprintf(f, "\tif (");
		if (iPacketLen > 0) {
			fprintf(f, "((struct %s::Packet*)pData)->m_header.p_length %s sizeof(struct ROM::Packet) + 0x%x && ",
			 oPacket->GetName(), (pLengthField == NULL) ? "==" : ">=", iPacketLen);
			if (pLengthField != NULL)
				fprintf(f, "((struct %s::Packet*)pData)->m_header.p_length == ((struct %s::Packet*)pData)->m_%s + sizeof(struct ROM::Packet) + 0x%x &&",
				 oPacket->GetName(), oPacket->GetName(), pLengthField->GetName(), iPacketLen);
		}
		fprintf(f, "%s) {\n", sCriterium);

		/*	
		 * If we have no subpackets, handle the command. Otherwise, the subpackets need to sort it out
		 */
		if (oPacket->GetSubpackets().empty()) {
			fprintf(f, "\t\t%s oRequest(pData, iDataLength);\n", oPacket->GetName());
			fprintf(f, "\t\tOn%s(oRequest);\n", oPacket->GetName());
			fprintf(f, "\t\treturn true;\n");
		}

		/* Now handle the subpackets */
		for (auto& oSubpacket: oPacket->GetSubpackets()) {
			char sPacketName[1024];
			sPacketName[snprintf(sPacketName, sizeof(sPacketName), "%s_%s", oPacket->GetName(), oSubpacket->GetName())] = '\0';
			if (!GenerateMatchCondition(oSubpacket->GetActions(), sPacketName, sCriterium, sizeof(sCriterium)))
				continue; // no fixed values

			// Determine subpacket length
			uint32_t iSubpacketLen = 0;
			bool bIsConstantSize = true;
			ProtocolDefinition::TransformationAction* pTransformation = NULL;
			for (auto& oAction: oSubpacket->GetActions()) {
				ProtocolDefinition::TransformationAction* pTT = dynamic_cast<ProtocolDefinition::TransformationAction*>(oAction);
				if (pTT != NULL)
					pTransformation = pTT;
				ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
				if (pField == NULL)
					continue;
				unsigned int iConstantSize = pField->GetType().GetConstantSize();
				iSubpacketLen += iConstantSize;
				bIsConstantSize &= iConstantSize != 0;
			}


			fprintf(f, "\t\tif (");
			if (iSubpacketLen > 0 && pLengthField != NULL && pTransformation == NULL /* skip length if a transformation */ && bIsConstantSize)
				fprintf(f, "((struct %s::Packet*)pData)->m_packet_%s == 0x%x && ", sPacketName, pLengthField->GetName(), iSubpacketLen);
			fprintf(f, "%s) {\n", sCriterium);
			fprintf(f, "\t\t\t%s oRequest(pData, iDataLength);\n", sPacketName);
			fprintf(f, "\t\t\tOn%s(oRequest);\n", sPacketName);
			fprintf(f, "\t\t\treturn true;\n");
			fprintf(f, "\t\t}\n");
		}

		fprintf(f, "\t}\n");
	}
	fprintf(f, "\treturn false; /* what's this? */\n");
	fprintf(f, "}\n");
}

void
ProtocolCodeGenerator::GenerateParserClass(FILE* f)
{
	fprintf(f, "class Parser {\n");
	fprintf(f, "public:\n");
	fprintf(f, "\tvirtual ~Parser() { };\n");
	fprintf(f, "\tvirtual bool Process(char* pData, int iDataLength);\n");
	fprintf(f, "\tvirtual void OnUnhandledPacket(const char* name) { }\n");
	fprintf(f, "\n");
	fprintf(f, "protected:\n");

	auto oPackets = m_Definition.GetPacketTypes();
	for (auto& oPacket: oPackets) {
#if 0
		// Skip anything not originating from the client
		if (oPacket->GetSource() != ProtocolDefinition::Packet::S_Client)
			continue;
#endif

		if (oPacket->GetSubpackets().empty())
			fprintf(f, "\tvirtual void On%s(%s& oPacket) { }\n", oPacket->GetName(), oPacket->GetName());

		for (auto& oSubpacket: oPacket->GetSubpackets()) {
			fprintf(f, "\tvirtual void On%s_%s(%s_%s& oPacket) { OnUnhandledPacket(\"%s_%s\"); }\n",
			 oPacket->GetName(), oSubpacket->GetName(),
			 oPacket->GetName(), oSubpacket->GetName(),
			 oPacket->GetName(), oSubpacket->GetName());
		}
	}

	fprintf(f, "};\n");
}

void
ProtocolCodeGenerator::GeneratePythonVariables(FILE* f, const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sCPrefix, const char* sPyPrefix, TNameTypePtrList& list)
{
	for (auto& oAction: oActions) {
		ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
		if (pField == NULL)
			continue; // XXX skips transformations

		const char* ctype = NULL;
		const char* parsetype = NULL;
		if (dynamic_cast<ProtocolDefinition::unsignedType*>(&pField->GetType()) != NULL) {
			ProtocolDefinition::unsignedType& ut = dynamic_cast<ProtocolDefinition::unsignedType&>(pField->GetType());
			if (ut.HaveFixedValue() || ut.GetCount() != 1)
				continue; // can't generate this
			ctype = "int"; parsetype = "I";
		} else if (dynamic_cast<ProtocolDefinition::lengthType*>(&pField->GetType()) != NULL) {
			ctype = "int"; parsetype = "I";
		} else if (dynamic_cast<ProtocolDefinition::unixtimeType*>(&pField->GetType()) != NULL) {
			ctype = "int"; parsetype = "I";
		} else if (dynamic_cast<ProtocolDefinition::floatType*>(&pField->GetType()) != NULL) {
			ctype = "float"; parsetype = "f";
		} else if (dynamic_cast<ProtocolDefinition::doubleType*>(&pField->GetType()) != NULL) {
			ctype = "double"; parsetype = "d";
		} else if (dynamic_cast<ProtocolDefinition::stringType*>(&pField->GetType()) != NULL) {
			ctype = "char*"; parsetype = "s";
		} else if (dynamic_cast<ProtocolDefinition::Struct*>(&pField->GetType()) != NULL) {
			ProtocolDefinition::Struct& s = dynamic_cast<ProtocolDefinition::Struct&>(pField->GetType());

			fprintf(f, "\t// struct: %s;\n", pField->GetType().GetName());
			fprintf(f, "\tROMPacket::%s %s;\n", pField->GetType().GetName(), pField->GetName());

			char cprefix[64], pyprefix[64];
			snprintf(cprefix, sizeof(cprefix), "%s_", pField->GetName());
			snprintf(pyprefix, sizeof(pyprefix), "%s.", pField->GetName());
			GeneratePythonVariables(f, s.GetActions(), cprefix, pyprefix, list);
			continue; // skip
		} else {
			abort(); /// FIXME
		} 

		fprintf(f, "\t%s %s%s;\n", ctype, sCPrefix, pField->GetName());

		char cname[64], pyname[64]; // XXX
		snprintf(cname, sizeof(cname), "%s%s", sCPrefix, pField->GetName());
		snprintf(pyname, sizeof(pyname), "%s%s", sPyPrefix, pField->GetName());
		list.push_back(new NameType(cname, pyname, parsetype));
	}
}

void
ProtocolCodeGenerator::GeneratePythonBindings(FILE* f)
{
	typedef std::list<char*> TCharPtrList;
	TCharPtrList bindings;

	auto oPackets = m_Definition.GetPacketTypes();
	for (auto& oPacket: oPackets) {
#if 0
		if (oPacket->GetSource() != ProtocolDefinition::Packet::S_Server)
			continue; // only generate server-based packets
#endif

		// XXX For the moment, we care only about subpackets
		for (auto& oSubpacket: oPacket->GetSubpackets()) {
			// XXX Skip anything with a transformation in them
			{
				const ProtocolDefinition::Struct::TXActionPtrList& oActions = oSubpacket->GetActions();
				ProtocolDefinition::Struct::TXActionPtrList::const_iterator oTransformAction = oActions.end();
				for (auto it = oActions.begin(); it != oActions.end(); it++) {
					if (dynamic_cast<ProtocolDefinition::TransformationAction*>(*it) == NULL)
						continue;
					// Okay, we found a transformation
					oTransformAction = it;
					break;
				}
				if (oTransformAction != oActions.end())
					continue; // we don't support these just yet
			}

			char* funcname = new char[256]; // XXX
			snprintf(funcname, 256, "%s_%s", oPacket->GetName(), oSubpacket->GetName());
			bindings.push_back(funcname);

			fprintf(f, "static PyObject*\n");
			fprintf(f, "%s_%s(PyObject* self, PyObject* args, PyObject* keywds)\n", oPacket->GetName(), oSubpacket->GetName());
			fprintf(f, "{\n");
			fprintf(f, "\n");

			// Generate fields
			TNameTypePtrList nametype_list;
			GeneratePythonVariables(f, oSubpacket->GetActions(), "", "", nametype_list);

			// Generate the python parser code
			fprintf(f, "\tstatic const char* kwlist[] = {");
			for (auto& oNameType: nametype_list) {
				NameType& nt = *oNameType;
				fprintf(f, " \"%s\",", nt.GetCName());
			}
			fprintf(f, " NULL };\n");
			fprintf(f, "\n");
			fprintf(f, "\tif (!PyArg_ParseTupleAndKeywords(args, keywds, \"");
			for (auto& oNameType: nametype_list) {
				NameType& nt = *oNameType;
				fprintf(f, "%s", nt.GetType());
			}
			fprintf(f, "\", (char**)kwlist");
			for (auto& oNameType: nametype_list) {
				NameType& nt = *oNameType;
				fprintf(f, ", &%s", nt.GetCName());
			}
			fprintf(f, "))\n");
			fprintf(f, "\t\treturn NULL;\n");

			fprintf(f, "\tGAME_BROADCAST({\n");
			fprintf(f, "\t\tROMPacket::%s_%s p;\n", oPacket->GetName(), oSubpacket->GetName());
			fprintf(f, "\t\tp.packet_target(targetid);\n"); // XXX KLUDGE
			for (auto& oNameType: nametype_list) {
				NameType& nt = *oNameType;
				if (strcmp(nt.GetType(), "s") == 0) {
					// XXX KLUDGE
					if (strchr(nt.GetVarName(), '.') != NULL) {
						fprintf(f, "\t\tstrcpy(&%s[0], %s);\n", nt.GetVarName(), nt.GetCName());
					} else {
						fprintf(f, "\t\tstrcpy(&p.%s()[0], %s);\n", nt.GetVarName(), nt.GetCName());
					}
				} else {
					// XXX KLUDGE
					if (strchr(nt.GetVarName(), '.') != NULL) {
						fprintf(f, "\t\t%s = %s;\n", nt.GetVarName(), nt.GetCName());
					} else {
						fprintf(f, "\t\tp.%s(%s);\n", nt.GetVarName(), nt.GetCName());
					}
				}
			}

			// Force structs in shape XXX ugly
			for (auto& oAction: oSubpacket->GetActions()) {
				ProtocolDefinition::Field* pField = dynamic_cast<ProtocolDefinition::Field*>(oAction);
				if (pField == NULL)
					continue; // XXX skips transformations
				ProtocolDefinition::Struct* s = dynamic_cast<ProtocolDefinition::Struct*>(&pField->GetType());
				if (s == NULL)
					continue;

				fprintf(f, "\t\tp.%s(%s);\n", pField->GetName(), pField->GetName());
			}

			fprintf(f, "\t\tp.Send(state);\n");
			fprintf(f, "\t})\n");

		
			fprintf(f, "\tPy_RETURN_NONE;\n");
			fprintf(f, "}\n");
			fprintf(f, "\n");

			for (auto& oNameType: nametype_list) {
				delete oNameType;
			}
		}
	}

	fprintf(f, "static PyMethodDef rompacket_methods[] = {\n");
	for (auto& b: bindings) {
		char* name = b;
		fprintf(f, "\t{ \"%s\", (PyCFunction)%s, METH_VARARGS | METH_KEYWORDS, \"\" },\n", name, name);
		delete[] name;
	}
	fprintf(f, "\t{ NULL, NULL, 0, NULL }\n");
	fprintf(f, "};\n");
	fprintf(f, "\n");

	fprintf(f, "void\n");
	fprintf(f, "init_bindings()\n");
	fprintf(f, "{\n");
	fprintf(f, "\tPy_InitModule(\"rompacket\", rompacket_methods);\n");
	fprintf(f, "}\n");
}

ProtocolCodeGenerator::NameType::NameType(const char* cname, const char* varname, const char* type)
{
	m_CName = strdup(cname);
	m_VarName = strdup(varname);
	m_Type = strdup(type);
}

ProtocolCodeGenerator::NameType::~NameType()
{	
	free(m_Type);
	free(m_VarName);
	free(m_CName);
}

/* vim:set ts=2 sw=2: */
