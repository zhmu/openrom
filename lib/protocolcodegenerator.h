/*
 * Runes of Magic protocol analysis - code generator
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
#ifndef __PROTOCOLCODEGENERATOR_H__
#define __PROTOCOLCODEGENERATOR_H__

#include "protocoldefinition.h"
#include <list>
#include <stdio.h>

class ProtocolDefinition;

class ProtocolCodeGenerator {
public:
	ProtocolCodeGenerator(ProtocolDefinition& oDefinition);

	void GenerateEnumerations(FILE* f);
	void GenerateTypes(FILE* f);
	void GeneratePackets(FILE* f);
	void GenerateFunctions(FILE* f);
	void GenerateParser(FILE* f, const char* sPacketPrefix);
	void GenerateParserClass(FILE* f);
	void GeneratePythonBindings(FILE* f);

private:
	ProtocolDefinition& m_Definition;

	void GenerateInitializers(FILE* f, const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sPrefix);
	void GenerateChangers(FILE* f, const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sPrefix, const char* sFuncPrefix);
	void GenerateFields(FILE* f, const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sPrefix);
	uint32_t GenerateLength(FILE* f, ProtocolDefinition::Packet& oPacket, ProtocolDefinition::Subpacket& oSubpacket, const char* sPrefix);

	bool GenerateMatchCondition(const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* sPacketName, char* sOutput, int iOutputLen);

	class NameType {
	public:
		NameType(const char* cname, const char* varname, const char* type);
		~NameType();

		const char* GetCName() { return m_CName; }
		const char* GetVarName() { return m_VarName; }
		const char* GetType() { return m_Type; }
	private:
		char* m_CName;
		char* m_VarName;
		char* m_Type;
	};
	typedef std::list<NameType*> TNameTypePtrList;

	void GeneratePythonVariables(FILE* f, const ProtocolDefinition::Struct::TXActionPtrList& oActions, const char* cprefix, const char* pyprefix, TNameTypePtrList& list);
};

#endif /* __PROTOCOLCODEGENERATOR_H__ */
