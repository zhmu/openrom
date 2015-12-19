/*
 * Runes of Magic protocol analysis - protocol decoding
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
#include "protocoldefinition.h"
#include <stdio.h>
#include <string.h>

bool
ProtocolDefinition::Field::Process(DecodeState& oState)
{
	m_DataOffset = oState.m_DataOffset;
	int r = m_Type->Fill(oState);
	if (r <= 0)
		return false;
	oState.m_Data += r;
	oState.m_DataLeft -= r;
	oState.m_DataOffset += r;
	return true;
}

int
ProtocolDefinition::Struct::GetConstantSize() const
{
	if (m_Count != m_MinCount)
		return 0; // field size may vary

	int iSize = 0;
	for (auto it = m_Actions.begin(); it != m_Actions.end(); it++) {
		Field* pField = dynamic_cast<Field*>(*it);
		if (pField == NULL)
			continue;
		int iFieldSize = pField->GetConstantSize();
		if (iFieldSize <= 0)
			return 0;
		iSize += iFieldSize;
	}
	return iSize * m_Count;
}

int
ProtocolDefinition::Struct::Fill(const DecodeState& oState)
{

	DecodeState oSubState(oState);
	oSubState.m_CurrentStruct = this;

	for (auto it = m_Actions.begin(); it != m_Actions.end(); it++) {
		XAction* pAction = *it;
		if (!pAction->Process(oSubState))
			break;
		//printf("ProtocolDefinition::Struct::Fill(): field='%s' offset=0x%x -> %d\n", oField.GetName(), iTotalFilled, iFieldFilled);
	}
	return oState.m_DataLeft - oSubState.m_DataLeft;
}

int
ProtocolDefinition::Packet::Fill(const DecodeState& oState)
{
	DecodeState oSubState(oState);

	m_LastSubpacket = NULL;
	int iNum = Struct::Fill(oSubState);
	if (iNum < 0)
		return 0;

	// If we have subpackets and the header doesn't check out, reject the packet
	if (!m_Subpackets.empty() && iNum != m_NumPacketBytes)
		return 0;

	// Try all subpackets
	oSubState.m_DataLeft -= m_NumPacketBytes;
	for (auto it = m_Subpackets.begin(); it != m_Subpackets.end(); it++) {
		Subpacket& oSP = **it;
		DecodeState oSubPacketState(oSubState);
		oSubPacketState.m_Data += m_NumPacketBytes;
		oSubPacketState.m_DataOffset += m_NumPacketBytes;
		int iAmount = oSP.Fill(oSubPacketState);
		if (iAmount == oSubPacketState.m_DataLeft) {
			iNum += oSubPacketState.m_DataLeft;
			m_LastSubpacket = &oSP;
			break;
		}
	//printf("no match '%s' %d <-> %d\n", oSP.GetName(), iAmount, oSubPacketState.m_DataLeft);
	}
	return iNum;
}

int
ProtocolDefinition::unsignedType::GetConstantSize() const
{
	if (m_Count != m_MinCount)
		return 0; // field size may vary
	return m_Count * m_Width;
}

int
ProtocolDefinition::unsignedType::Fill(const DecodeState& oState)
{
	if (m_Width * m_MinCount > oState.m_DataLeft)
		return 0; // too little data

	// See how many entries we have
	int num = oState.m_DataLeft / m_Width;
	if (num > m_Count)
		num = m_Count;

	// Fetch the values
	const uint8_t* pData = oState.m_Data;
	for (int n = 0; n < num; n++) {
		m_Value[n] = *pData++;
		if (m_Width > 1)
			m_Value[n] |= *pData++ << 8;
		if (m_Width > 2)
			m_Value[n] |= *pData++ << 16;
		if (m_Width > 3)
			m_Value[n] |= *pData++ << 24;
	}

	// If we need to correspond with a fixed value, check it
	if (m_HaveFixedValue && m_Value[0] != m_FixedValue)
		return -1;

	// Value read
	return m_Width * num;
}

int
ProtocolDefinition::stringType::GetConstantSize() const
{
	if (m_MinLength != m_Length)
		return 0; // not constant size

	return m_Length;
}

int
ProtocolDefinition::stringType::Fill(const DecodeState& oState)
{
	if (oState.m_DataLeft < m_MinLength)
		return 0; // not enough data

	int iLength = m_Length;
	if (iLength > oState.m_DataLeft)
		iLength = oState.m_DataLeft; 

	memcpy(&m_Data[0], oState.m_Data, iLength);
	return iLength;
}

int
ProtocolDefinition::floatType::GetConstantSize() const
{
	return m_Count * sizeof(float);
}

int
ProtocolDefinition::floatType::Fill(const DecodeState& oState)
{
	if (oState.m_DataLeft < m_Count * sizeof(float))
		return 0;

	const uint8_t* pData = oState.m_Data;
	for (int n = 0; n < m_Count; n++) {
		uint32_t v = *pData++;
		v |= *pData++ << 8;
		v |= *pData++ << 16;
		v |= *pData++ << 24;
		m_Number[n] = *(float*)&v;
	}
	return m_Count * sizeof(float);
}

int
ProtocolDefinition::doubleType::GetConstantSize() const
{
	return m_Count * sizeof(double);
}

int
ProtocolDefinition::doubleType::Fill(const DecodeState& oState)
{
	if (oState.m_DataLeft < m_Count * sizeof(double))
		return 0;

	const uint8_t* pData = oState.m_Data;
	for (int n = 0; n < m_Count; n++) {
		uint64_t v = (uint64_t)*pData++;
		v |= (uint64_t)*pData++ << 8;
		v |= (uint64_t)*pData++ << 16;
		v |= (uint64_t)*pData++ << 24;
		v |= (uint64_t)*pData++ << 32;
		v |= (uint64_t)*pData++ << 40;
		v |= (uint64_t)*pData++ << 48;
		v |= (uint64_t)*pData++ << 56;
		m_Number[n] = *(double*)&v;
	}
	return m_Count * sizeof(double);
}

int
ProtocolDefinition::lengthType::GetConstantSize() const
{
	return sizeof(uint32_t);
}

int
ProtocolDefinition::lengthType::Fill(const DecodeState& oState)
{
	if (oState.m_DataLeft < sizeof(uint32_t))
		return 0;

	// Read the u32 of data
	const uint8_t* pData = oState.m_Data;
	m_Value = *pData++;
	m_Value |= *pData++ << 8;
	m_Value |= *pData++ << 16;
	m_Value |= *pData++ << 24;

	// Add our own length
	m_Value += sizeof(uint32_t);
	if (m_Value != oState.m_DataLeft) {
		printf("ProtocolDefinition::lengthType::Fill(): rejecting, got %u, left %u\n", m_Value, oState.m_DataLeft);
		return 0;
	}
	return sizeof(uint32_t);
}

int
ProtocolDefinition::unixtimeType::GetConstantSize() const
{
	return sizeof(uint32_t);
}

int
ProtocolDefinition::unixtimeType::Fill(const DecodeState& oState)
{
	if (oState.m_DataLeft < sizeof(uint32_t))
		return 0;

	// Read the u32 of data
	const uint8_t* pData = oState.m_Data;
	m_Value = *pData++;
	m_Value |= *pData++ << 8;
	m_Value |= *pData++ << 16;
	m_Value |= *pData++ << 24;
	return sizeof(uint32_t);
}

ProtocolDefinition::Packet*
ProtocolDefinition::Process(const uint8_t* pData, int iLength)
{
	DecodeState oState;
	oState.m_Data = pData;
	oState.m_DataLeft = iLength;

	for (auto it = m_Packet.begin(); it != m_Packet.end(); it++) {
		Packet* pPacket = *it;
		int iProcessed = pPacket->Fill(oState);
		if (iProcessed == iLength)
			return pPacket;
	}
	return NULL;
}

/* vim:set ts=2 sw=2: */
