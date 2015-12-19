/*
 * Runes of Magic protocol analysis - protocol display
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
#include <string>
#include "dataannotation.h"
	
void
ProtocolDefinition::Struct::GetHumanReadableContent(char* out, int outlen) const
{
	snprintf(out, outlen, "<struct>");
	out[outlen - 1] = '\0';
}

void
ProtocolDefinition::Struct::Print(int iIndent) const
{
	printf("struct '%s'\n", m_Name);
	PrintFields(iIndent + 1);
}

void
ProtocolDefinition::Struct::PrintFields(int iIndent) const
{
	for (auto it = m_Actions.begin(); it != m_Actions.end(); it++) {
		Field* pField = dynamic_cast<Field*>(*it);
		if (pField == NULL)
			continue;
		ProtocolDefinition::PrintIndent(iIndent);
			printf("'%s'", pField->GetName());
		if (ProtocolDefinition::MustPrintDataOffset())
			printf(" @ 0x%x", pField->GetDataOffset());
		printf(": ");
		pField->GetType().Print(iIndent + 1);
		// kludge to prevent newline after final member; Print() generally does that
		if (*it != m_Actions.back())
			printf("\n");
	}
}

void
ProtocolDefinition::Subpacket::GetHumanReadableContent(char* out, int outlen) const
{
	snprintf(out, outlen, "<subpacket>");
	out[outlen - 1] = '\0';
}

void
ProtocolDefinition::Subpacket::Print(int iIndent) const
{
	ProtocolDefinition::PrintIndent(iIndent); printf("subpacket '%s'\n", m_Name);
	PrintFields(iIndent + 1);
}

void
ProtocolDefinition::Packet::GetHumanReadableContent(char* out, int outlen) const
{
	snprintf(out, outlen, "<packet>");
	out[outlen - 1] = '\0';
}

void
ProtocolDefinition::Packet::Print(int iIndent) const
{
	ProtocolDefinition::PrintIndent(iIndent); printf("packet '%s'\n", m_Name);
	PrintFields(iIndent + 1);
	if (m_LastSubpacket != NULL)
		m_LastSubpacket->Print(iIndent + 1);
}

void
ProtocolDefinition::unsignedType::GetHumanReadableContent(char* out, int outlen) const
{
	out[outlen - 1] = '\0'; // ensure \0-termination

	if (m_Enumeration != NULL) {
		Enumeration::TValue sValue = m_Enumeration->Lookup(m_Value[0]);
		// Be careful: if we also have an annotation configured and the enumeration doesn't work, pass it through
		if (sValue != NULL || m_Annotation == NULL) {
			snprintf(out, outlen - 1, m_Format == F_DECIMAL ? "%s <%u>" : "%s <0x%x>", sValue != NULL ? sValue : "?", m_Value[0]);
			return;
		}
	}
	if (m_Annotation != NULL) {
		const char* sValue = m_Annotation->GetProvider().Lookup(m_Value[0]);
		snprintf(out, outlen - 1, m_Format == F_DECIMAL ? "%s <%u>" : "%s <0x%x>", sValue, m_Value[0]);
		return;
	}

	if (m_Count != 1) {
		snprintf(out, outlen - 1, "<length %d>", m_Count);
	} else {
		snprintf(out, outlen - 1, m_Format == F_DECIMAL ? "%u" : "0x%x", m_Value[0]);
	}
}

void
ProtocolDefinition::unsignedType::Print(int iIndent) const
{
	if (m_Count == 1) {
		char tmp[256];
		GetHumanReadableContent(tmp, sizeof(tmp));
		printf("%s", tmp);
	} else {
		int iCount = m_Count;
		if (m_DisplayCount >= 0)
			iCount = m_DisplayCount;
		for (int n = 0; n < iCount; n++) {
			if (n > 0)
				printf(" ");
			printf(m_Format == F_DECIMAL ? "%u" : "0x%x", m_Value[n]);
		}
	}
}

void
ProtocolDefinition::signedType::GetHumanReadableContent(char* out, int outlen) const
{
	// In case of non-decimal or enumerations, assume the user meant unsigned
	if (m_Format != F_DECIMAL || m_Enumeration != NULL || m_Count != 1) {
		ProtocolDefinition::unsignedType::GetHumanReadableContent(out, outlen);
		return;
	}
	out[outlen - 1] = '\0'; // ensure \0-termination
	snprintf(out, outlen - 1, "%d", m_Value[0]);
}

void
ProtocolDefinition::signedType::Print(int iIndent) const
{
	// In case of non-decimal or enumerations, assume the user meant unsigned
	if (m_Format != F_DECIMAL || m_Enumeration != NULL || m_Count != 1) {
		ProtocolDefinition::unsignedType::Print(iIndent);
		return;
	}

	int iCount = m_Count;
	if (m_DisplayCount >= 0)
		iCount = m_DisplayCount;
	for (int n = 0; n < iCount; n++) {
		if (n > 0)
			printf(" ");
		printf(m_Format == F_DECIMAL ? "%d" : "0x%x", m_Value[n]);
	}
}

void
ProtocolDefinition::stringType::GetHumanReadableContent(char* out, int outlen) const
{
	out[outlen - 1] = '\0'; // ensure \0-termination
	snprintf(out, outlen - 1, "'%s'", m_Data);
}

void
ProtocolDefinition::stringType::Print(int iIndent) const
{
	printf("'");
	for (unsigned int n = 0; n < m_Length; n++) {
		if (m_Data[n] == '\0')
			break;
		printf("%c", m_Data[n]);
	}
	printf("'");
}

void
ProtocolDefinition::floatType::GetHumanReadableContent(char* out, int outlen) const
{
	out[outlen - 1] = '\0'; // ensure \0-termination
	if (m_Count != 1) {
		snprintf(out, outlen - 1, "<length %d>", m_Count);
	} else {
		snprintf(out, outlen - 1, "%.2f", m_Number[0]);
	}
}

void
ProtocolDefinition::floatType::Print(int iIndent) const
{
	int iCount = m_Count;
	if (m_DisplayCount >= 0)
		iCount = m_DisplayCount;
	for (int n = 0; n < iCount; n++) {
		if (n > 0)
			printf(" ");
		printf("%.2f", m_Number[n]);
	}
}

void
ProtocolDefinition::doubleType::GetHumanReadableContent(char* out, int outlen) const
{
	out[outlen - 1] = '\0'; // ensure \0-termination
	if (m_Count != 1) {
		snprintf(out, outlen - 1, "<length %d>", m_Count);
	} else {
		snprintf(out, outlen - 1, "%g", m_Number[0]);
	}
}

void
ProtocolDefinition::doubleType::Print(int iIndent) const
{
	int iCount = m_Count;
	if (m_DisplayCount >= 0)
		iCount = m_DisplayCount;
	for (int n = 0; n < iCount; n++) {
		if (n > 0)
			printf(" ");
		printf("%g", m_Number[n]);
	}
}

void
ProtocolDefinition::lengthType::GetHumanReadableContent(char* out, int outlen) const
{
	out[outlen - 1] = '\0'; // ensure \0-termination
	snprintf(out, outlen - 1, "%x", m_Value);
}

void
ProtocolDefinition::lengthType::Print(int iIndent) const
{
	printf("%x", m_Value);
}

void
ProtocolDefinition::unixtimeType::GetHumanReadableContent(char* out, int outlen) const
{
	out[outlen - 1] = '\0'; // ensure \0-termination
	snprintf(out, outlen - 1, "%x", m_Value);
}

void
ProtocolDefinition::unixtimeType::Print(int iIndent) const
{
	time_t t = m_Value;
	struct tm tm;
	if (gmtime_r(&t, &tm) != NULL) {
		printf("%04d-%02d-%02d %02d:%02d:%02d <%d>",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		 m_Value);
	} else {
		printf("? <%d>", m_Value);
	}
}

void
ProtocolDefinition::PrintIndent(int iIndent)
{
	for (int n = 0; n < iIndent; n++)
		printf(" ");
}

/* vim:set ts=2 sw=2: */
