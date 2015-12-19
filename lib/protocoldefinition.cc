/*
 * Runes of Magic protocol analysis - protocol definition
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
#include <libxml/tree.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h> // for free()
#include "dataannotation.h"
#include "datatransformation.h"

bool ProtocolDefinition::s_MustPrintDataOffset = false;

ProtocolDefinition::Definition::Definition(const char* sName, const char* sValue)
{
	m_Name = strdup(sName);
	m_Value = strdup(sValue);
}

ProtocolDefinition::Definition::~Definition()
{
	free(m_Value);
	free(m_Name);
}

ProtocolDefinition::Transformation::Transformation(const char* sName, XDataTransformation& oTransformation)
{
	m_Name = strdup(sName);
	m_TransformationProvider = &oTransformation;
}

ProtocolDefinition::Transformation::~Transformation()
{
	delete m_TransformationProvider;
	free(m_Name);
}

ProtocolDefinition::Annotation::Annotation(const char* sName, XDataAnnotation& oAnnotation)
{
	m_Name = strdup(sName);
	m_AnnotationProvider = &oAnnotation;
}

ProtocolDefinition::Annotation::~Annotation()
{
	delete m_AnnotationProvider;
	free(m_Name);
}

ProtocolDefinition::Enumeration::Enumeration(const char* sName)
{
	m_Name = strdup(sName);
}

ProtocolDefinition::Enumeration::~Enumeration()
{
	for(auto it = m_Map.begin(); it != m_Map.end(); it++)
		delete it->second;
	free(m_Name);
}

bool
ProtocolDefinition::Enumeration::Add(TKey tKey, const TValue tValue)
{
	auto result = m_Map.insert(std::pair<TKey, TValue>(tKey, NULL));
	if (!result.second)
		return false; // already present
	
	result.first->second = strdup(tValue);
	return true;
}

const ProtocolDefinition::Enumeration::TValue
ProtocolDefinition::Enumeration::Lookup(TKey tKey) const
{
	auto it = m_Map.find(tKey);
	if (it == m_Map.end())
		return NULL;
	return it->second;
}

ProtocolDefinition::Type::Type(ProtocolDefinition& oProtocolDefinition, const char* sName)
	: m_ProtocolDefinition(oProtocolDefinition)
{
	m_Name = strdup(sName);
}

ProtocolDefinition::Type::~Type()
{
	free(m_Name);
}

ProtocolDefinition::Field::Field(const Type& oType, const char* sName)
{
	m_Name = strdup(sName);
	m_Type = oType.Clone();
}

ProtocolDefinition::XAction*
ProtocolDefinition::Field::Clone() const
{
	return new Field(*m_Type, m_Name);
}

ProtocolDefinition::Field::~Field()
{
	delete m_Type;
	free(m_Name);
}

ProtocolDefinition::TransformationAction::TransformationAction(Transformation& oTransformation)
	: m_Transformation(oTransformation), m_Buffer(NULL)
{
}

ProtocolDefinition::TransformationAction::~TransformationAction()
{
	delete[] m_Buffer;
}

ProtocolDefinition::XAction*
ProtocolDefinition::TransformationAction::Clone() const
{
	return new TransformationAction(m_Transformation);
}

bool
ProtocolDefinition::TransformationAction::Process(DecodeState& oState)
{
	XDataTransformation& oTransformation = m_Transformation.GetProvider();

	// Throw away any buffer we had
	delete[] m_Buffer;
	m_Buffer = NULL;

	// First of all, see if we can figure out the buffer size to use; if this
	// fails, we can't do anything at all
	int iBufferSize = oTransformation.EstimateBufferSize(oState.m_Data, oState.m_DataLeft);
	if (iBufferSize <= 0)
		return false;

	m_Buffer = new uint8_t[iBufferSize];
	if (!oTransformation.Apply(oState.m_Data, oState.m_DataLeft, m_Buffer, iBufferSize))
		return false;

	oState.m_Data = m_Buffer;
	oState.m_DataOffset = 0; // Reset offset
	oState.m_DataLeft = iBufferSize;
	return true;
}

ProtocolDefinition::AnnotationAction::AnnotationAction(Annotation& oAnnotation)
	: m_Annotation(oAnnotation)
{
}

ProtocolDefinition::XAction*
ProtocolDefinition::AnnotationAction::Clone() const
{
	return new AnnotationAction(m_Annotation);
}

bool
ProtocolDefinition::AnnotationAction::Process(DecodeState& oState)
{
	XDataAnnotation& oAnnotation = m_Annotation.GetProvider();

	oAnnotation.Apply(oState.m_CurrentStruct);
	return true;
}

ProtocolDefinition::Struct::Struct(ProtocolDefinition& oProtocolDefinition, const char* sName)
	: Type(oProtocolDefinition, sName), m_Count(1), m_MinCount(1)
{
}

ProtocolDefinition::Struct::~Struct()
{
	for (auto it = m_Actions.begin(); it != m_Actions.end(); it++)
		delete *it;
}

void
ProtocolDefinition::Struct::AddAction(XAction* pAction)
{
	m_Actions.push_back(pAction);
}

ProtocolDefinition::Type*
ProtocolDefinition::Struct::Clone() const
{
	Struct* pStruct = new Struct(m_ProtocolDefinition, m_Name);
	pStruct->m_Count = m_Count;
	pStruct->m_MinCount = m_MinCount;
	for (auto it = m_Actions.begin(); it != m_Actions.end(); it++)
		pStruct->AddAction((*it)->Clone());
	return pStruct;
}

void
ProtocolDefinition::Struct::GenerateCType(char* sType, char* sSuffix) const
{
	snprintf(sType, s_GenerateMaxLength, "%s", m_Name);
	strcpy(sSuffix, "");
}

void
ProtocolDefinition::Struct::GenerateCInitialize(char* sCode, int iLength) const
{
	int iCurrent = 0;
	strcpy(sCode, "");
	for (auto it = m_Actions.begin(); it != m_Actions.end(); it++) {
		Field* pField = dynamic_cast<Field*>(*it);
		if (pField == NULL)
			continue; // skips transformations!
		pField->GetType().GenerateCInitialize(sCode + iCurrent, iLength - iCurrent);
		iCurrent += strlen(sCode + iCurrent);
	}
}

bool
ProtocolDefinition::Struct::ParseExtraNode(xmlNodePtr pNode, const char* sName)
{
	if (xmlStrcmp(pNode->name, (const xmlChar*)"transformation") == 0) {
		Transformation* pTransformation = m_ProtocolDefinition.LookupTransformation(sName);
		if (pTransformation == NULL) {
			fprintf(stderr, "ProtocolDefinition::Struct::ParseExtraNode(): transformation '%s' not recognized\n", sName);
			return false;
		}
		TransformationAction* pTransformationAction = new TransformationAction(*pTransformation);
		AddAction(pTransformationAction);
		return true;
	}
	if (xmlStrcmp(pNode->name, (const xmlChar*)"annotation") == 0) {
		Annotation* pAnnotation = m_ProtocolDefinition.LookupAnnotation(sName);
		if (pAnnotation == NULL) {
			fprintf(stderr, "ProtocolDefinition::Struct::ParseExtraNode(): annotation '%s' not recognized\n", sName);
			return false;
		}
		AnnotationAction* pAnnotationAction = new AnnotationAction(*pAnnotation);
		AddAction(pAnnotationAction);
		return true;
	}

	xmlChar* sCount = xmlGetProp(pNode, (const xmlChar*)"count");
	if (sCount != NULL) {
		intmax_t iVal;
		if (!m_ProtocolDefinition.ResolveStringToNumber((const char*)sCount, iVal))
			return false;
		// If the minimum count is equal to the count, honor that here too - it
		// will be overwritten by the 'min_count' parsing code below
		if (m_Count == m_MinCount)
			m_MinCount = iVal;
		m_Count = iVal;
	}

	xmlChar* sMinCount = xmlGetProp(pNode, (const xmlChar*)"min_count");
	if (sMinCount != NULL) {
		intmax_t iVal;
		if (!m_ProtocolDefinition.ResolveStringToNumber((const char*)sMinCount, iVal))
			return false;
		m_MinCount = iVal;
		xmlFree(sMinCount);
	}

	return false;
}

bool
ProtocolDefinition::Struct::ParseNode(xmlNodePtr pNode)
{
	bool bOK = true;
	for (pNode = pNode->children; bOK && pNode != NULL; pNode = pNode->next) {
		if (pNode->type != XML_ELEMENT_NODE)
			continue;

		bOK = false;
		xmlChar* sName = xmlGetProp(pNode, (const xmlChar*)"name");
		if (sName == NULL) {
			fprintf(stderr, "ProtocolDefinition::Struct::ParseNode(): missing name attribute\n");
			break;
		}

		xmlChar* sType = NULL;
		if (xmlStrcmp(pNode->name, (const xmlChar*)"field") == 0) {
			sType = xmlGetProp(pNode, (const xmlChar*)"type");
			if (sType != NULL) {
				const Type* poType = m_ProtocolDefinition.LookupType((const char*)sType);
				if (poType == NULL) {
					fprintf(stderr, "ProtocolDefinition::Struct::ParseNode(): type '%s' not recognized (name '%s')\n", sType, sName);
					break;
				}

				// Hook the type up to the struct
				Field* pField = new Field(*poType, (const char*)sName);
				AddAction(pField);
				bOK = pField->GetType().ParseNode(pNode);
			} else { 
				fprintf(stderr, "ProtocolDefinition::Struct::ParseNode(): missing type attribute\n");
				break;
			}
		} else if (ParseExtraNode(pNode, (const char*)sName)) {
			bOK = true;
		} else {
			fprintf(stderr, "ProtocolDefinition::Struct::ParseNode(): found unrecognized node '%s'\n", pNode->name);
			break;
		}
		if (sType != NULL)
			xmlFree(sType);
		if (sName != NULL)
			xmlFree(sName);
	}

	return bOK;
}

ProtocolDefinition::Packet::Packet(ProtocolDefinition& oProtocolDefinition, const char* sName)
 : Struct(oProtocolDefinition, sName), m_NumPacketBytes(0), m_Source(S_Unknown)
{
}

ProtocolDefinition::Packet::~Packet()
{
	for (auto it = m_Subpackets.begin(); it != m_Subpackets.end(); it++)
		delete *it;
}

bool
ProtocolDefinition::Packet::ParseNode(xmlNodePtr pNode)
{
	if (!ProtocolDefinition::Struct::ParseNode(pNode))
		return false;

	xmlChar* sSource = xmlGetProp(pNode, (const xmlChar*)"source");
	if (sSource == NULL) {
		fprintf(stderr, "ProtocolDefinition::Packet::ParseNode(): missing source attribute\n");
		return false;
	}
	if (xmlStrcmp(sSource, (const xmlChar*)"server") == 0)
		m_Source = S_Server;
	else if (xmlStrcmp(sSource, (const xmlChar*)"client") == 0)
		m_Source = S_Client;
	xmlFree(sSource);
	
	if (m_Source == S_Unknown) {
		fprintf(stderr, "ProtocolDefinition::Packet::ParseNode(): packet '%s' has unrecognized source\n", m_Name);
		return false;
	}

	m_NumPacketBytes = 0;
	for (auto it = m_Actions.begin(); it != m_Actions.end(); it++) {
		Field* pField = dynamic_cast<Field*>(*it);
		if (pField == NULL)
			continue;
		int iFieldSize = pField->GetType().GetConstantSize();
		if (iFieldSize <= 0) {
			m_NumPacketBytes = 0;
			break;
		}
		m_NumPacketBytes += iFieldSize;
	}

	if (!m_Subpackets.empty() && m_NumPacketBytes == 0) {
		fprintf(stderr, "ProtocolDefinition::Packet::ParseNode(): packet '%s' has non-constant field size or no members\n", m_Name);
		return false;
	}
	return true;
}

bool
ProtocolDefinition::Packet::ParseExtraNode(xmlNodePtr pNode, const char* sName)
{
	if (xmlStrcmp(pNode->name, (const xmlChar*)"subpacket") != 0)
		return false;

	Subpacket* pSubpacket = new Subpacket(m_ProtocolDefinition, sName);
	m_Subpackets.push_back(pSubpacket);
	return pSubpacket->ParseNode(pNode);
}

ProtocolDefinition::unsignedType::unsignedType(ProtocolDefinition& oProtocolDefinition, const char* sName, int iWidth)
 : BuiltinType(oProtocolDefinition, sName), m_Width(iWidth), m_Enumeration(NULL), m_Annotation(NULL), m_HaveFixedValue(false), m_FixedValue(0), m_Count(1), m_MinCount(1), m_DisplayCount(-1),
   m_Format(F_HEX)
{
	assert(iWidth >= 1 && iWidth <= sizeof(m_Value));
	m_Value = new uint32_t[m_Count];
}

ProtocolDefinition::unsignedType::~unsignedType()
{
	delete[] m_Value;
}

uint32_t
ProtocolDefinition::unsignedType::GetValue(int n) const
{
	if (n < 0 || n >= m_Count)
		return 0;
	return m_Value[n];
}

ProtocolDefinition::Type*
ProtocolDefinition::unsignedType::Clone() const
{
	unsignedType* pType = new unsignedType(m_ProtocolDefinition, m_Name, m_Width);
	pType->m_Value = new uint32_t[m_Count];
	pType->m_Count = m_Count;
	pType->m_DisplayCount = m_DisplayCount;
	pType->m_MinCount = m_MinCount;
	pType->m_Enumeration = m_Enumeration;
	pType->m_Annotation = m_Annotation;
	pType->m_HaveFixedValue = m_HaveFixedValue;
	pType->m_FixedValue = m_FixedValue;
	pType->m_Format = m_Format;
	return pType;
}

void
ProtocolDefinition::unsignedType::GenerateCType(char* sType, char* sSuffix) const
{
	if (m_Count > 1) {
		snprintf(sType, s_GenerateMaxLength, "%s", m_Name);
		snprintf(sSuffix, s_GenerateMaxLength, "[%d]", m_Count);
	} else {
		snprintf(sType, s_GenerateMaxLength, "%s", m_Name);
		strcpy(sSuffix, "");
	}
}

void
ProtocolDefinition::unsignedType::GenerateCInitialize(char* sCode, int iLength) const
{
	strcpy(sCode, "");
	if (!m_HaveFixedValue)
		return;
	assert(m_Count == 1);
	snprintf(sCode, iLength, (m_Format == F_HEX) ? "0x%x" : "%d", m_FixedValue);
}

bool
ProtocolDefinition::unsignedType::ParseNode(xmlNodePtr pNode)
{
	bool bOK = true;

	xmlChar* sFixedValue = xmlGetProp(pNode, (const xmlChar*)"fixed_value");
	if (sFixedValue != NULL) {
		intmax_t iFixVal;
		bOK &= m_ProtocolDefinition.ResolveStringToNumber((const char*)sFixedValue, iFixVal);
		m_HaveFixedValue = true;
		m_FixedValue = iFixVal;
		xmlFree(sFixedValue);
	}

	xmlChar* sCount = xmlGetProp(pNode, (const xmlChar*)"count");
	if (sCount != NULL) {
		intmax_t iVal;
		bOK &= m_ProtocolDefinition.ResolveStringToNumber((const char*)sCount, iVal);
		if (bOK) {
			// If the minimum count is equal to the count, honor that here too - it
			// will be overwritten by the 'min_count' parsing code below
			if (m_Count == m_MinCount)
				m_MinCount = iVal;
			m_Count = iVal;
			delete[] m_Value;
			m_Value = new uint32_t[m_Count];
		}
		xmlFree(sCount);
	}

	xmlChar* sMinCount = xmlGetProp(pNode, (const xmlChar*)"min_count");
	if (sMinCount != NULL) {
		intmax_t iVal;
		bOK &= m_ProtocolDefinition.ResolveStringToNumber((const char*)sMinCount, iVal);
		m_MinCount = iVal;
		xmlFree(sMinCount);
	}

	xmlChar* sDisplayCount = xmlGetProp(pNode, (const xmlChar*)"display");
	if (sDisplayCount != NULL) {
		intmax_t iVal;
		bOK &= m_ProtocolDefinition.ResolveStringToNumber((const char*)sDisplayCount, iVal);
		m_DisplayCount = iVal;
		xmlFree(sDisplayCount);
	}

	xmlChar* sFormat = xmlGetProp(pNode, (const xmlChar*)"format");
	if (sFormat != NULL) {
		if (xmlStrcmp(sFormat, (const xmlChar*)"hex") == 0)
			m_Format = F_HEX;
		if (xmlStrcmp(sFormat, (const xmlChar*)"decimal") == 0)
			m_Format = F_DECIMAL;
		else {
			fprintf(stderr, "ProtocolDefinition::unsignedType::ParseNode(): undefined format '%s'\n", sFormat);
			bOK = false;
		}
		xmlFree(sDisplayCount);
	}

	xmlChar* sEnumeration = xmlGetProp(pNode, (const xmlChar*)"enumeration");
	if (sEnumeration != NULL) {
		const Enumeration* pEnum = m_ProtocolDefinition.LookupEnumeration((const char*)sEnumeration);
		if (pEnum == NULL) {
			fprintf(stderr, "ProtocolDefinition::unsignedType::ParseNode(): undefined enumeration '%s'\n", sEnumeration);
			bOK = false;
		}
		m_Enumeration = pEnum;
		xmlFree(sEnumeration);
	}

	xmlChar* sAnnotation = xmlGetProp(pNode, (const xmlChar*)"annotation");
	if (sAnnotation != NULL) {
		const Annotation* pAnno = m_ProtocolDefinition.LookupAnnotation((const char*)sAnnotation);
		if (pAnno == NULL) {
			fprintf(stderr, "ProtocolDefinition::unsignedType::ParseNode(): undefined annotation '%s'\n", sAnnotation);
			bOK = false;
		}
		m_Annotation = pAnno;
		xmlFree(sAnnotation);
	}

	return bOK;
}

ProtocolDefinition::signedType::signedType(ProtocolDefinition& oProtocolDefinition, const char* sName, int iWidth)
	: unsignedType(oProtocolDefinition, sName, iWidth)
{
}

ProtocolDefinition::Type*
ProtocolDefinition::signedType::Clone() const
{
	// XXX This is unfortunate; identical to unsignedType ...
	signedType* pType = new signedType(m_ProtocolDefinition, m_Name, m_Width);
	pType->m_Value = new uint32_t[m_Count];
	pType->m_Count = m_Count;
	pType->m_DisplayCount = m_DisplayCount;
	pType->m_MinCount = m_MinCount;
	pType->m_Enumeration = m_Enumeration;
	pType->m_Annotation = m_Annotation;
	pType->m_HaveFixedValue = m_HaveFixedValue;
	pType->m_FixedValue = m_FixedValue;
	pType->m_Format = m_Format;
	return pType;
}

ProtocolDefinition::stringType::stringType(ProtocolDefinition& oProtocolDefinition)
 : BuiltinType(oProtocolDefinition, "string"), m_Data(NULL)
{
}

ProtocolDefinition::stringType::~stringType()
{
	delete[] m_Data;
}

ProtocolDefinition::Type*
ProtocolDefinition::stringType::Clone() const
{
	stringType* pType = new stringType(m_ProtocolDefinition);
	pType->m_Data = new char[m_Length];
	pType->m_MinLength = m_MinLength;
	pType->m_Length = m_Length;
	return pType;
}

void
ProtocolDefinition::stringType::GenerateCType(char* sType, char* sSuffix) const
{
	snprintf(sType, s_GenerateMaxLength, "char");
	snprintf(sSuffix, s_GenerateMaxLength, "[%d]", m_Length);
}

void
ProtocolDefinition::stringType::GenerateCInitialize(char* sCode, int iLength) const
{
	strcpy(sCode, "");
}

bool
ProtocolDefinition::stringType::ParseNode(xmlNodePtr pNode)
{
	bool bOK = true;

	xmlChar* sLength = xmlGetProp(pNode, (const xmlChar*)"length");
	if (sLength != NULL) {
		intmax_t iVal;
		bOK = m_ProtocolDefinition.ResolveStringToNumber((const char*)sLength, iVal);
		m_Length = iVal;
		m_MinLength = iVal; // may be overwritten later
		m_Data = new char[m_Length];
		xmlFree(sLength);
	} else {
		fprintf(stderr, "ProtocolDefinition::stringType::ParseNode(): name '%s' without 'length', aborting\n", m_Name);
		return false;
	}

	xmlChar* sMinLength = xmlGetProp(pNode, (const xmlChar*)"min_length");
	if (sMinLength != NULL) {
		intmax_t iVal;
		bOK = bOK && m_ProtocolDefinition.ResolveStringToNumber((const char*)sMinLength, iVal);
		m_MinLength = iVal;
		xmlFree(sMinLength);
	}

	return bOK;
}

ProtocolDefinition::floatType::floatType(ProtocolDefinition& oProtocolDefinition)
 : BuiltinType(oProtocolDefinition, "float"), m_Count(1), m_DisplayCount(-1)
{
	m_Number = new float[m_Count];
}

ProtocolDefinition::floatType::~floatType()
{
	delete[] m_Number;
}

ProtocolDefinition::Type*
ProtocolDefinition::floatType::Clone() const
{
	floatType* pType = new floatType(m_ProtocolDefinition);
	pType->m_Number = new float[m_Count];
	pType->m_Count = m_Count;
	pType->m_DisplayCount = m_DisplayCount;
	return pType;
}

void
ProtocolDefinition::floatType::GenerateCType(char* sType, char* sSuffix) const
{
	if (m_Count > 1) {
		snprintf(sType, s_GenerateMaxLength, "%s", m_Name);
		snprintf(sSuffix, s_GenerateMaxLength, "[%d]", m_Count);
	} else {
		snprintf(sType, s_GenerateMaxLength, "%s", m_Name);
		strcpy(sSuffix, "");
	}
}

void
ProtocolDefinition::floatType::GenerateCInitialize(char* sCode, int iLength) const
{
	strcpy(sCode, "");
}

bool
ProtocolDefinition::floatType::ParseNode(xmlNodePtr pNode)
{
	bool bOK = true;

	xmlChar* sCount = xmlGetProp(pNode, (const xmlChar*)"count");
	if (sCount != NULL) {
		intmax_t iVal;
		bOK &= m_ProtocolDefinition.ResolveStringToNumber((const char*)sCount, iVal);
		if (bOK) {
			m_Count = iVal;
			delete[] m_Number;
			m_Number = new float[m_Count];
		}
		xmlFree(sCount);
	}

	xmlChar* sDisplayCount = xmlGetProp(pNode, (const xmlChar*)"display");
	if (sDisplayCount != NULL) {
		intmax_t iVal;
		bOK &= m_ProtocolDefinition.ResolveStringToNumber((const char*)sDisplayCount, iVal);
		m_DisplayCount = iVal;
		xmlFree(sDisplayCount);
	}
	return bOK;
}

ProtocolDefinition::doubleType::doubleType(ProtocolDefinition& oProtocolDefinition)
 : BuiltinType(oProtocolDefinition, "double"), m_Count(1), m_DisplayCount(-1)
{
	m_Number = new double[m_Count];
}

ProtocolDefinition::doubleType::~doubleType()
{
	delete[] m_Number;
}

ProtocolDefinition::Type*
ProtocolDefinition::doubleType::Clone() const
{
	doubleType* pType = new doubleType(m_ProtocolDefinition);
	pType->m_Number = new double[m_Count];
	pType->m_Count = m_Count;
	pType->m_DisplayCount = m_DisplayCount;
	return pType;
}

void
ProtocolDefinition::doubleType::GenerateCType(char* sType, char* sSuffix) const
{
	if (m_Count > 1) {
		snprintf(sType, s_GenerateMaxLength, "%s", m_Name);
		snprintf(sSuffix, s_GenerateMaxLength, "[%d]", m_Count);
	} else {
		snprintf(sType, s_GenerateMaxLength, "%s", m_Name);
		strcpy(sSuffix, "");
	}
}

void
ProtocolDefinition::doubleType::GenerateCInitialize(char* sCode, int iLength) const
{
	strcpy(sCode, "");
}

bool
ProtocolDefinition::doubleType::ParseNode(xmlNodePtr pNode)
{
	bool bOK = true;

	xmlChar* sCount = xmlGetProp(pNode, (const xmlChar*)"count");
	if (sCount != NULL) {
		intmax_t iVal;
		bOK &= m_ProtocolDefinition.ResolveStringToNumber((const char*)sCount, iVal);
		if (bOK) {
			m_Count = iVal;
			delete[] m_Number;
			m_Number = new double[m_Count];
		}
		xmlFree(sCount);
	}

	xmlChar* sDisplayCount = xmlGetProp(pNode, (const xmlChar*)"display");
	if (sDisplayCount != NULL) {
		intmax_t iVal;
		bOK &= m_ProtocolDefinition.ResolveStringToNumber((const char*)sDisplayCount, iVal);
		m_DisplayCount = iVal;
		xmlFree(sDisplayCount);
	}
	return bOK;
}

ProtocolDefinition::Type*
ProtocolDefinition::lengthType::Clone() const
{
	return new lengthType(m_ProtocolDefinition);
}

bool
ProtocolDefinition::lengthType::ParseNode(xmlNodePtr pNode)
{
	// Nothing to parse
	return true;
}

void
ProtocolDefinition::lengthType::GenerateCType(char* sType, char* sSuffix) const
{
	// XXX We override the type here to prevent problems with fields called 'length'
	snprintf(sType, s_GenerateMaxLength, "ulength");
	strcpy(sSuffix, "");
}

void
ProtocolDefinition::lengthType::GenerateCInitialize(char* sCode, int iLength) const
{
	strcpy(sCode, "");
}

ProtocolDefinition::Type*
ProtocolDefinition::unixtimeType::Clone() const
{
	return new unixtimeType(m_ProtocolDefinition);
}

bool
ProtocolDefinition::unixtimeType::ParseNode(xmlNodePtr pNode)
{
	// Nothing to parse
	return true;
}

void
ProtocolDefinition::unixtimeType::GenerateCType(char* sType, char* sSuffix) const
{
	snprintf(sType, s_GenerateMaxLength, m_Name);
	strcpy(sSuffix, "");
}

void
ProtocolDefinition::unixtimeType::GenerateCInitialize(char* sCode, int iLength) const
{
	strcpy(sCode, "");
}

ProtocolDefinition::ProtocolDefinition()
{
	m_Types.push_back(new unsignedType(*this, "u8", sizeof(uint8_t)));
	m_Types.push_back(new unsignedType(*this, "u16", sizeof(uint16_t)));
	m_Types.push_back(new unsignedType(*this, "u32", sizeof(uint32_t)));
	m_Types.push_back(new signedType(*this, "s8", sizeof(int8_t)));
	m_Types.push_back(new signedType(*this, "s16", sizeof(int16_t)));
	m_Types.push_back(new signedType(*this, "s32", sizeof(int32_t)));
	m_Types.push_back(new stringType(*this));
	m_Types.push_back(new floatType(*this));
	m_Types.push_back(new doubleType(*this));
	m_Types.push_back(new lengthType(*this));
	m_Types.push_back(new unixtimeType(*this));
}

ProtocolDefinition::~ProtocolDefinition()
{
	for (auto it = m_Packet.begin(); it != m_Packet.end(); it++)
		delete *it;
	for (auto it = m_Enums.begin(); it != m_Enums.end(); it++)
		delete *it;
	for (auto it = m_Definitions.begin(); it != m_Definitions.end(); it++)
		delete *it;
	for (auto it = m_Transformations.begin(); it != m_Transformations.end(); it++)
		delete *it;
	for (auto it = m_Types.begin(); it != m_Types.end(); it++)
		delete *it;
}

const ProtocolDefinition::Type*
ProtocolDefinition::LookupType(const char* sName) const
{
	for (auto it = m_Types.begin(); it != m_Types.end(); it++) {
		if (strcmp((*it)->GetName(), sName) != 0)
			continue;
		return *it;
	}
	return NULL;
}

const ProtocolDefinition::Enumeration*
ProtocolDefinition::LookupEnumeration(const char* sName) const
{
	for (auto it = m_Enums.begin(); it != m_Enums.end(); it++) {
		if (strcmp((*it)->GetName(), sName) != 0)
			continue;
		return *it;
	}
	return NULL;
}

ProtocolDefinition::Annotation*
ProtocolDefinition::LookupAnnotation(const char* sName) const
{
	for (auto it = m_Annotations.begin(); it != m_Annotations.end(); it++) {
		if (strcmp((*it)->GetName(), sName) != 0)
			continue;
		return *it;
	}
	return NULL;
}

const ProtocolDefinition::Definition*
ProtocolDefinition::LookupDefinition(const char* sName) const
{
	for (auto it = m_Definitions.begin(); it != m_Definitions.end(); it++) {
		if (strcmp((*it)->GetName(), sName) != 0)
			continue;
		return *it;
	}
	return NULL;
}

ProtocolDefinition::Transformation*
ProtocolDefinition::LookupTransformation(const char* sName) const
{
	for (auto it = m_Transformations.begin(); it != m_Transformations.end(); it++) {
		if (strcmp((*it)->GetName(), sName) != 0)
			continue;
		return *it;
	}
	return NULL;
}

void
ProtocolDefinition::RegisterTransformation(const char* sName, XDataTransformation& oDataTransformation)
{
	m_Transformations.push_back(new Transformation(sName, oDataTransformation));
}

void
ProtocolDefinition::RegisterAnnotation(const char* sName, XDataAnnotation& oAnnotation)
{
	m_Annotations.push_back(new Annotation(sName, oAnnotation));
}

bool
ProtocolDefinition::ResolveStringToNumber(const char* sString, intmax_t& iNumber)
{
	const char* sValue = sString;

	// Attempt to resolve a definition, first
	const Definition* pDefinition = LookupDefinition(sString);
	if (pDefinition != NULL)
		sValue = pDefinition->GetValue();

	char* pPtr;
	iNumber = (intmax_t)strtol(sValue, &pPtr, 0);
	if (*pPtr != '\0') {
		if (pDefinition != NULL)
			fprintf(stderr, "ProtocolDefinition::ResolveStringToNumber(): can't resolve '%s' (definition of '%s') to number\n", sValue, sString);
		else 
			fprintf(stderr, "ProtocolDefinition::ResolveStringToNumber(): can't resolve '%s' to number\n", sValue);
		return false;
	}
	return true;
}

bool
ProtocolDefinition::ParseEnum(xmlNodePtr pNode)
{
	// Fetch the node name
	xmlChar* sName = xmlGetProp(pNode, (const xmlChar*)"name");
	if (sName == NULL) {
		fprintf(stderr, "ProtocolDefinition::ParseEnum(): no 'name' attribute'\n");
		return false;
	}

	// Create the enumeration
	Enumeration* pEnum = new Enumeration((const char*)sName);
	m_Enums.push_back(pEnum);
	xmlFree(sName);

	// Walk through the children nodes; these must be value -> name pairs
	bool bOK = true;
	for (pNode = pNode->children; pNode != NULL; pNode = pNode->next) {
		if (pNode->type != XML_ELEMENT_NODE)
			continue;

		bOK = false;
		if (xmlStrcmp(pNode->name, (const xmlChar*)"value") != 0) {
			fprintf(stderr, "ProtocolDefinition::ParseEnum(): found unrecognized node '%s'\n", pNode->name);
			break;
		}

		xmlChar* sNum = xmlGetProp(pNode, (const xmlChar*)"num");
		xmlChar* sName = xmlGetProp(pNode, (const xmlChar*)"name");
		if (sNum != NULL && sName != NULL) {
			char* sPtr;
			int iNum = (int)strtol((const char*)sNum, &sPtr, 0);
			if (*sPtr != '\0') {
				fprintf(stderr, "ProtocolDefinition::ParseEnum(): non-numeric '%s', aborting\n", sNum);
				break;
			}
			if (!pEnum->Add(iNum, (const Enumeration::TValue)sName)) {
				fprintf(stderr, "ProtocolDefinition::ParseEnum(): duplicate enumeration key %d, aborting\n", iNum);
				break;
			}

			// This enum value is okay
			bOK = true;
		} else {
			fprintf(stderr, "ProtocolDefinition::ParseEnum(): node without num/value, aborting\n");
		}
		if (sName != NULL)
			xmlFree(sName);
		if (sNum != NULL)
			xmlFree(sNum);
		if (!bOK)
			break; // error should have been shown already
	}

	return bOK;
}

bool
ProtocolDefinition::ParseDefinition(xmlNodePtr pNode)
{
	bool bOK = false;

	// Fetch the definition name and value
	xmlChar* sName = xmlGetProp(pNode, (const xmlChar*)"name");
	if (sName != NULL) {
		if (LookupDefinition((const char*)sName) == NULL) {
			xmlChar* sValue = xmlGetProp(pNode, (const xmlChar*)"value");
			if (sValue != NULL) {
				Definition* pDefinition = new Definition((const char*)sName, (const char*)sValue);
				m_Definitions.push_back(pDefinition);
				xmlFree(sValue);
				bOK = true;
			} else {
				fprintf(stderr, "ProtocolDefinition::ParseDefinition(): no 'name' attribute'\n");
			}
		} else {
			fprintf(stderr, "ProtocolDefinition::ParseDefinition(): duplicate definition '%s', aborting\n", sName);
		}
		xmlFree(sName);
	} else {
		fprintf(stderr, "ProtocolDefinition::ParseDefinition(): no 'name' attribute'\n");
	}

	return bOK;
}

bool
ProtocolDefinition::ParseStruct(xmlNodePtr pNode)
{
	// Fetch the node name
	xmlChar* sName = xmlGetProp(pNode, (const xmlChar*)"name");
	if (sName == NULL) {
		fprintf(stderr, "ProtocolDefinition::ParseStruct(): no 'name' attribute'\n");
		return false;
	}

	// Create the structured type
	Struct* pStruct = new Struct(*this, (const char*)sName);
	m_Types.push_back(pStruct);
	xmlFree(sName);

	return pStruct->ParseNode(pNode);
}

bool
ProtocolDefinition::ParsePacket(xmlNodePtr pNode)
{
	// Fetch the node name
	xmlChar* sName = xmlGetProp(pNode, (const xmlChar*)"name");
	if (sName == NULL) {
		fprintf(stderr, "ProtocolDefinition::ParsePacket(): no 'name' attribute'\n");
		return false;
	}

	// Create the packet type
	Packet* pPacket = new Packet(*this, (const char*)sName);
	m_Packet.push_back(pPacket);
	xmlFree(sName);

	return pPacket->ParseNode(pNode);
}

bool
ProtocolDefinition::Load(const char* sFilename)
{
	xmlDocPtr pDoc = xmlReadFile(sFilename, NULL, 0);
	if (pDoc == NULL) {
		printf("ProtocolDefinition::Load(): cannot parse '%s'\n", sFilename);
		return false;
	}

	bool bOK = true;
	try {
		for (xmlNodePtr pNode = xmlDocGetRootElement(pDoc)->children; bOK && pNode != NULL; pNode = pNode->next) {
			if (pNode->type != XML_ELEMENT_NODE)
				continue;

			if (xmlStrcmp(pNode->name, (const xmlChar*)"enum") == 0) {
				bOK &= ParseEnum(pNode);
				continue;
			}
			if (xmlStrcmp(pNode->name, (const xmlChar*)"struct") == 0) {
				bOK &= ParseStruct(pNode);
				continue;
			}
			if (xmlStrcmp(pNode->name, (const xmlChar*)"packet") == 0) {
				bOK &= ParsePacket(pNode);
				continue;
			}
			if (xmlStrcmp(pNode->name, (const xmlChar*)"define") == 0) {
				bOK &= ParseDefinition(pNode);
				continue;
			}

			fprintf(stderr, "ProtocolDefinition::Load(): unrecognized node '%s', aborting\n", pNode->name);
			bOK = false;
		}
	} catch(...) {
	}

	xmlFreeDoc(pDoc);
	return bOK;
}

/* vim:set ts=2 sw=2: */
