/*
 * Runes of Magic protocol analysis - protocol definition and decoding
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
#ifndef __PROTOCOLDEFINITION_H__
#define __PROTOCOLDEFINITION_H__

#include <map>
#include <list>
#include <stdint.h> // for uintXX_t

typedef struct _xmlNode xmlNode;
typedef xmlNode* xmlNodePtr;

class XDataTransformation;
class XDataAnnotation;

class ProtocolDefinition {
	friend class ProtocolCodeGenerator;
public:
	//! \brief Maximum generated C++ string
	static const int s_GenerateMaxLength = 64;

	//! \brief Enumeration of values
	class Enumeration {
		friend class ProtocolCodeGenerator;
	public:
		/*! \brief Creates an enumeration with a given name
		 *  \param sName Name to use
		 */
		Enumeration(const char* sName);
		~Enumeration();

		//! \brief Retrieve the enumeration name
		const char* GetName() const { return m_Name; }

		typedef int TKey;
		typedef char* TValue;

		/*! \brief Adds an enumeration value
		 *  \param tKey Number to use
		 *  \param tValue Value to bind (will be copied)
		 *  \returns true if a new value was added
		 */
		bool Add(TKey tKey, const TValue tValue);

		/*! \brief Looks up a given key
		 *  \param eKey Key to look up
		 *  \returns Value or NULL if not found
		 */
		const TValue Lookup(TKey tKey) const;

	protected:
		typedef std::map<TKey, TValue> TKeyValueMap;

		//! \brief Retrieves all enumeration values
		const TKeyValueMap& GetValueMap() const { return m_Map; }

		//! \brief Enumeration name
		char* m_Name;

		//! \brief Map containing all enumeration values
		TKeyValueMap m_Map;
	};

	//! \brief Definition
	class Definition {
		friend class ProtocolCodeGenerator;
	public:
		/*! \brief Creates a new definition
		 *  \param sName Definition name
		 *  \param sValue Definition value
		 */
		Definition(const char* sName, const char* sValue);
		~Definition();

		//! \brief Retrieve the definition name
		const char* GetName() const { return m_Name; }

		//! \brief Retrieve the definition value
		const char* GetValue() const { return m_Value; }

	protected:
		//! \brief Definition name
		char* m_Name;

		//! \brief Definition value
		char* m_Value;
	};

	//! \brief Data transformation
	class Transformation {
	public:
		/*! \brief Creates a new transformation 
		 *  \param sName Transformation name
		 *  \param oTransformation Transformation provider
		 *
		 *  Note that this class will become owner of the
		 *  transformation provider and destroy it as needed.
		 */
		Transformation(const char* sName, XDataTransformation& oTransformation);
		~Transformation();

		//! \brief Retrieve the definition name
		const char* GetName() const { return m_Name; }

		//! \brief Retrieves the transformation provider
		XDataTransformation& GetProvider() const { return *m_TransformationProvider; }

	protected:
		//! \brief Transformation name
		char* m_Name;

		//! \brief Transformation provider
		XDataTransformation* m_TransformationProvider;
	};

	//! \brief Data annotation
	class Annotation {
	public:
		/*! \brief Creates a new annotation 
		 *  \param sName Annotation name
		 *  \param oAnnotation Annotation provider
		 *
		 *  Note that this class will become owner of the
		 *  annotation provider and destroy it as needed.
		 */
		Annotation(const char* sName, XDataAnnotation& oAnnotation);
		~Annotation();

		//! \brief Retrieve the definition name
		const char* GetName() const { return m_Name; }

		//! \brief Retrieves the transformation provider
		XDataAnnotation& GetProvider() const { return *m_AnnotationProvider; }

	protected:
		//! \brief Annotation name
		char* m_Name;

		//! \brief Annotation provider
		XDataAnnotation* m_AnnotationProvider;
	};

	class DecodeState;

	//! \brief Describes a given type
	class Type {
	public:
		/*! \brief Creates the type with a given name
		 *  \param oDefinition Protocol definition we belong to
		 *  \param sName Name to use
		 */
		Type(ProtocolDefinition& oDefinition, const char* sName);

		//! \brief Destroys the type
		virtual ~Type();

		//! \brief Retrieve the type's name
		const char* GetName() const { return m_Name; }

		//! \brief Creates a copy of the type
		virtual Type* Clone() const = 0;

		/*! \brief Display type content
		 *  \param iIndent Indentation to use
		 */
		virtual void Print(int iIndent) const = 0;

		/*! \brief Retrieve the content in a human-readable fashion
		 *  \param out Output to place the content in
		 *  \param outlen Number of bytes to write at most
		 */
		virtual void GetHumanReadableContent(char* out, int outlen) const = 0;

		/*! \brief Parses the structured type from an XML node
		 *  \param pNode Node to parse
		 *  \returns true on success
		 *
		 *  This function is expected to show the error on failure.
		 */
		virtual bool ParseNode(xmlNodePtr pNode) = 0;

		/*! \brief Fills the type
		 *  \param oState Decoding state to use
		 *  \returns Number of bytes processed or -1 on failure
		 */
		virtual int Fill(const DecodeState& oState) = 0;

		/*! \brief Retrieve the type size, in bytes
		 *  \returns Field size, or 0 if the field size is not constant
		 */
		virtual int GetConstantSize() const = 0;

		/*! \brief Generates the C++ type
		 *  \param sType C++ type identifier
		 *  \param sSuffix Suffix to use
		 *
		 *  The output must be reasonable (i.e. within ProtocolDefinition::s_GenerateMaxLength bytes)
		 */
		virtual void GenerateCType(char* sType, char* sSuffix) const = 0;

		/*! \brief Generate C++ initializer code
		 *  \param sCode Code to fill
		 *  \param iLength Length in bytes to use at most
		 */
		virtual void GenerateCInitialize(char* sCode, int iLength) const = 0;

	protected:
		//! \brief Protocol definition we belong to
		ProtocolDefinition& m_ProtocolDefinition;

		//! \brief Name of the type
		char* m_Name;
	};

	class Struct;

	//! \brief Decoder state
	class DecodeState {
	public:
		//! \brief Data to decode
		const uint8_t* m_Data;

		//! \brief Number of data bytes left
		int m_DataLeft;

		//! \brief Current data offset, used for display purposes
		uint32_t m_DataOffset;

		/*! \brief Current structure we are at
		 *
		 *  XXX This is a kludge to have annotations be able to look up things
		 */
		Struct* m_CurrentStruct;
	};

	//! \brief Interface of a decode action
	class XAction {
	public:
		/*! \brief Processes input
		 *  \param oState State to use and update
		 *  \returns true on success
		 */
		virtual bool Process(DecodeState& oState) = 0;

		//! \brief Creates a copy of the action
		virtual XAction* Clone() const = 0;
	};

	//! \brief Field of a given value
	class Field : public XAction {
	public:
		/*! \brief Creates a new field
		 *  \param oType Type to use
		 *  \param sName Name to use for the field
		 */
		Field(const Type& oType, const char* sName);

		//! \brief Copies the field
		virtual XAction* Clone() const;

		//! \brief Destroys the field
		~Field();

		//! \brief Retrieve the field name
		const char* GetName() const { return m_Name; }

		//! \brief Retrieve the field type
		Type& GetType() { return *m_Type; }
		const Type& GetType() const { return *m_Type; }

		/*! \brief Retrieve the field size, in bytes
		 *  \returns Field size, or 0 if the field size is not constant
		 */
		int GetConstantSize() const { return m_Type->GetConstantSize(); }

		//! \brief Offset where decoded, in bytes
		uint32_t GetDataOffset() const { return m_DataOffset; }

		virtual bool Process(DecodeState& oState);

	protected:
		//! \brief Name of the field
		char* m_Name;

		//! \brief Type of the field
		Type* m_Type;

		//! \brief Data offset, in bytes
		uint32_t m_DataOffset;

		//! \brief Assignment is forbidden
		Field& operator=(const Field& oField) = delete;
	};

	//! \brief Applies a transformation action
	class TransformationAction : public XAction {
	public:
		/*! \brief Creates a new transformation action
		 *  \param oTransformation Transformation to apply
		 */
		TransformationAction(Transformation& oTransformation);
		virtual ~TransformationAction();

		virtual XAction* Clone() const;
		virtual bool Process(DecodeState& oState);
		const Transformation& GetTransformation() const { return m_Transformation; }

	protected:
		//! \brief Transformation to use
		Transformation& m_Transformation;

		//! \brief Decode buffer
		uint8_t* m_Buffer;
	};

	//! \brief Applies a annotation action
	class AnnotationAction : public XAction {
	public:
		/*! \brief Creates a new annotation action
		 *  \param oAnnotation Annotation to apply
		 */
		AnnotationAction(Annotation& oAnnotation);

		virtual XAction* Clone() const;
		virtual bool Process(DecodeState& oState);
		const Annotation& GetAnnotation() const { return m_Annotation; }

	protected:
		//! \brief Annotation to use
		Annotation& m_Annotation;
	};

	//! \brief Structured type
	class Struct : public Type {
		friend class ProtocolCodeGenerator;
	public:
		/*! \brief Creates the type with a given name
		 *  \param sName Name to use
		 */
		Struct(ProtocolDefinition& oProtocolDefinition, const char* sName);
		~Struct();

		/*! \brief Adds an action to the type
		 *  \param pField Field to add
		 *
		 *  The object will consider itself owner of the action.
		 */
		void AddAction(XAction* pAction);

		virtual Type* Clone() const;
		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);

		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		virtual void PrintFields(int iIndent) const;
		virtual int GetConstantSize() const;
		virtual void GenerateCType(char* sType, char* sSuffix) const;
		virtual void GenerateCInitialize(char* sCode, int iLength) const;

		typedef std::list<XAction*> TXActionPtrList;

		//! \brief Retrieves all actions within the struct
		const TXActionPtrList& GetActions() const { return m_Actions; }

	protected:

		/*! \brief Called to parse an extra node type
		 *  \param pNode Node to parse
		 *  \param sName Node name attribute
		 *  \returns true on success
		 *
		 *  "Extra" nodes are those that are implemented only by
		 *  derived classes.
		 */
		virtual bool ParseExtraNode(xmlNodePtr pNode, const char* sName);

		/*! \brief Parses the actual child node
		 *  \param pNode Node to parse
		 *  \returns true on success
		 */
		bool ParseChildNode(xmlNodePtr pNode);

		TXActionPtrList m_Actions;

		//! \brief Number of values
		int m_Count;

		//! \brief Minimum count
		int m_MinCount;
	};

	//! \brief Subpacket type
	class Subpacket : public Struct {
	public:
		Subpacket(ProtocolDefinition& oProtocolDefinition, const char* sName) : Struct(oProtocolDefinition, sName) { }
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
	};

	//! \brief Packet type
	class Packet : public Struct {
		friend class ProtocolCodeGenerator;
	public:
		Packet(ProtocolDefinition& oProtocolDefinition, const char* sName);
		~Packet();

		enum Source {
			S_Unknown,
			S_Server,
			S_Client
		};
		Source GetSource() const { return m_Source; }

		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		const Subpacket* GetSubpacket() const { return m_LastSubpacket; }

	protected:
		Source m_Source;
		virtual bool ParseExtraNode(xmlNodePtr pNode, const char* sName);

		typedef std::list<Subpacket*> TSubpacketPtrList;
		TSubpacketPtrList m_Subpackets;

		//! \brief Retrieve all subpackets
		const TSubpacketPtrList& GetSubpackets() const { return m_Subpackets; }

		//! \brief Size of packet header bytes
		int m_NumPacketBytes;

		//! \brief Most recent subpacket identified
		Subpacket* m_LastSubpacket;
	};

	ProtocolDefinition();
	~ProtocolDefinition();

	/*! \brief Loads protocol definitions from a file
	 *  \param sFilename Filename to use
	 *  \param iVersion Version to load, -1 for latest
	 *  \returns true on success
	 */
	bool Load(const char* sFilename, int iVersion);

	/*! \brief Processes a decrypted packet payload
	 *  \param pData Data to process
	 *  \param iLength Length to process
	 *  \returns Packet on success, or NULL
	 */
	Packet* Process(const uint8_t* pData, int iLength);

	/*! \brief Registers a transformation
	 *  \param sName Transformation name
	 *  \param oDataTransformation Backing object to use
	 *
	 *  Note that the object becomes owner of the backing transformation
	 *  object and will delete it as needed.
	 */
	void RegisterTransformation(const char* sName, XDataTransformation& oDataTransformation);

	/*! \brief Registers an annotation
	 *  \param sName Annotation name
	 *  \param oAnnotation Backing object to use
	 *
	 *  Note that the object becomes owner of the backing annotation 
	 *  object and will delete it as needed.
	 */
	void RegisterAnnotation(const char* sName, XDataAnnotation& oAnnotation);

	//! \brief Built-in type, which needn't be parsed
	class BuiltinType : public Type {
	protected:
		BuiltinType(ProtocolDefinition& oProtocolDefinition, const char* sName) : Type(oProtocolDefinition, sName) { }
	};

	//! \brief Basic unsigned numeric type
	class unsignedType : public BuiltinType {
	public:
		unsignedType(ProtocolDefinition& oProtocolDefinition, const char* sName, int iWidth);
		virtual ~unsignedType();
		virtual Type* Clone() const;
		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		virtual int GetConstantSize() const;
		virtual void GenerateCType(char* sType, char* sSuffix) const;
		virtual void GenerateCInitialize(char* sCode, int iLength) const;

		// XXX This is a kludge for romdump
		uint32_t GetValue(int n) const;
		bool HaveFixedValue() const { return m_HaveFixedValue; }
		int GetCount() const { return m_Count; }

	protected:
		//! \brief Type width, in bytes
		int m_Width;

		//! \brief Number of values
		int m_Count;

		//! \brief Minimum count
		int m_MinCount;

		//! \brief Number of values to display
		int m_DisplayCount;

		//! \brief Backing value (large so everything will fit)
		uint32_t* m_Value;

		//! \brief Is a fixed value given?
		bool m_HaveFixedValue;

		//! \brief What is the fixed value?
		uint32_t m_FixedValue;

		//! \brief Enumeration belonging to the values, if any
		const Enumeration* m_Enumeration;

		//! \brief Annotation belonging to the values, if any
		const Annotation* m_Annotation;

		//! \brief How to display
		enum Format {
			F_HEX,
			F_DECIMAL
		};
		Format m_Format;
	};

	//! \brief Basic signed numeric type
	class signedType : public unsignedType {
	public:
		signedType(ProtocolDefinition& oProtocolDefinition, const char* sName, int iWidth);
		virtual Type* Clone() const;
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
	};

	//! \brief length type
	class lengthType : public BuiltinType {
	public:
		lengthType(ProtocolDefinition& oProtocolDefinition) : BuiltinType(oProtocolDefinition, "length") { }
		virtual Type* Clone() const;
		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		virtual int GetConstantSize() const;
		virtual void GenerateCType(char* sType, char* sSuffix) const;
		virtual void GenerateCInitialize(char* sCode, int iLength) const;

	private:
		//! \brief Value read
		uint32_t m_Value;
	};

	//! \brief UNIX timestamp type, 4 bytes
	class unixtimeType : public BuiltinType {
	public:
		unixtimeType(ProtocolDefinition& oProtocolDefinition) : BuiltinType(oProtocolDefinition, "unixtime") { }
		virtual Type* Clone() const;
		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		virtual int GetConstantSize() const;
		virtual void GenerateCType(char* sType, char* sSuffix) const;
		virtual void GenerateCInitialize(char* sCode, int iLength) const;

	private:
		//! \brief Value read
		uint32_t m_Value;
	};

	//! \brief UNIX timestamp type, 4 bytes
	class unixtime4Type : public BuiltinType {
	public:
		unixtime4Type(ProtocolDefinition& oProtocolDefinition) : BuiltinType(oProtocolDefinition, "unixtime4") { }
		virtual Type* Clone() const;
		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		virtual int GetConstantSize() const;
		virtual void GenerateCType(char* sType, char* sSuffix) const;
		virtual void GenerateCInitialize(char* sCode, int iLength) const;

	private:
		//! \brief Value read
		uint32_t m_Value;
	};


	//! \brief string type
	class stringType : public BuiltinType {
	public:
		stringType(ProtocolDefinition& oProtocolDefinition);
		virtual ~stringType();

		virtual Type* Clone() const;
		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		virtual int GetConstantSize() const;
		virtual void GenerateCType(char* sType, char* sSuffix) const;
		virtual void GenerateCInitialize(char* sCode, int iLength) const;

		// XXX This is a kludge for romdump
		const char* GetValue() const { return m_Data; }

	private:
		//! \brief String length
		int m_Length;

		//! \brief Minimal string length
		int m_MinLength;

		//! \brief String data
		char* m_Data;
	};

	//! \brief float type
	class floatType : public BuiltinType {
	public:
		floatType(ProtocolDefinition& oProtocolDefinition);
		virtual ~floatType();
		virtual Type* Clone() const;
		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		virtual int GetConstantSize() const;
		virtual void GenerateCType(char* sType, char* sSuffix) const;
		virtual void GenerateCInitialize(char* sCode, int iLength) const;

	private:
		//! \brief Float data
		float* m_Number;

		//! \brief Number of values
		int m_Count;

		//! \brief Number of values to display
		int m_DisplayCount;
	};

	//! \brief double type
	class doubleType : public BuiltinType {
	public:
		doubleType(ProtocolDefinition& oProtocolDefinition);
		virtual ~doubleType();
		virtual Type* Clone() const;
		virtual int Fill(const DecodeState& oState);
		virtual bool ParseNode(xmlNodePtr pNode);
		virtual void Print(int iIndent) const;
		virtual void GetHumanReadableContent(char* out, int outlen) const;
		virtual int GetConstantSize() const;
		virtual void GenerateCType(char* sType, char* sSuffix) const;
		virtual void GenerateCInitialize(char* sCode, int iLength) const;

	private:
		//! \brief Float data
		double* m_Number;

		//! \brief Number of values
		int m_Count;

		//! \brief Number of values to display
		int m_DisplayCount;
	};

	//! \brief Are data byte offsets to be printed?
	static bool MustPrintDataOffset() { return s_MustPrintDataOffset; }
	
	/*! \brief Set whether to print data offsets during printing
	 *  \param b true to print offsets
	 */
	static bool SetPrintDataOffset(bool b) { s_MustPrintDataOffset = b; }

protected:
	/*! \brief Looks a type up by name
	 *  \param sName Name to look up
	 *  \returns Type on success or NULL
	 */
	const Type* LookupType(const char* sName) const;

	/*! \brief Looks a enumeration up by name
	 *  \param sName Name to look up
	 *  \returns Enumeration on success or NULL
	 */
	const Enumeration* LookupEnumeration(const char* sName) const;

	/*! \brief Looks an annotation up by name
	 *  \param sName Name to look up
	 *  \returns Annotation on success or NULL
	 */
	Annotation* LookupAnnotation(const char* sName) const;

	/*! \brief Looks a definition up by name
	 *  \param sName Name to look up
	 *  \returns Definition on success or NULL
	 */
	const Definition* LookupDefinition(const char* sName) const;

	/*! \brief Looks a transformation up by name
	 *  \param sName Name to look up
	 *  \returns Transformation on success or NULL
	 */  
	Transformation* LookupTransformation(const char* sName) const;

	/*! \brief Resolves a string to a number
	 *  \param sString String to resolve
	 *  \param iNumber Resulting number
	 *  \returns true on success
	 */
	bool ResolveStringToNumber(const char* sString, intmax_t& iNumber);

	/*! \brief Indents to a given level
	 *  \param iIndent Level to use
	 */
	static void PrintIndent(int iLevel);

	bool ParseEnum(xmlNodePtr pNode);
	bool ParseStruct(xmlNodePtr pNode);
	bool ParsePacket(xmlNodePtr pNode);
	bool ParseDefinition(xmlNodePtr pNode);

	typedef std::list<Type*> TTypePtrList;
	typedef std::list<Enumeration*> TEnumerationPtrList;
	typedef std::list<Annotation*> TAnnotationPtrList;
	typedef std::list<Packet*> TPacketPtrList;
	typedef std::list<Definition*> TDefinitionPtrList;
	typedef std::list<Transformation*> TTransformationPtrList;

	//! \brief All registered definitions
	TDefinitionPtrList m_Definitions;

	//! \brief Fetches all registered definitions
	const TDefinitionPtrList& GetDefinitions() const { return m_Definitions; }

	//! \brief All registered types
	TTypePtrList m_Types;

	//! \brief Fetches all registered types 
	const TTypePtrList& GetTypes() const { return m_Types; }

	//! \brief All registered enumerations 
	TEnumerationPtrList m_Enums;

	//! \brief All registered annotations 
	TAnnotationPtrList m_Annotations;

	//! \brief Fetches all registered enumerations 
	const TEnumerationPtrList& GetEnumerations() const { return m_Enums; }

	//! \brief All registered transformations
	TTransformationPtrList m_Transformations;

	//! \brief Fetches all registered transformations 
	const TTransformationPtrList& GetTransformations() const { return m_Transformations; }

	//! \brief All registered packet types
	TPacketPtrList m_Packet;

	//! \brief Fetches all registered packet types
	const TPacketPtrList& GetPacketTypes() const { return m_Packet; }

	/*! \brief Whether to print data offsets
	 *
	 *  This is here because all Type::Print() functions need to access it...
	 */
	static bool s_MustPrintDataOffset;

	//!  \brief Version to load
	int m_Version;
};

#endif /* __PROTOCOLDEFINITION_H__ */
