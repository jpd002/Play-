#include <string.h>
#include "StructFile.h"
#include "xml/Node.h"
#include "lexical_cast_ex.h"

#define STRUCT_FIELD ("Field")
#define STRUCT_FIELD_NAME ("Name")
#define STRUCT_FIELD_VALUE ("Value")

using namespace Framework;
using namespace std;

CStructFile::CStructFile()
{
}

CStructFile::~CStructFile()
{
}

void CStructFile::Read(Xml::CNode* rootNode)
{
	Xml::CNode::NodeList nodeList = rootNode->SelectNodes(STRUCT_FIELD);
	for(Xml::CNode::NodeIterator nodeIterator(nodeList.begin());
	    nodeIterator != nodeList.end(); nodeIterator++)
	{
		Xml::CNode* node = (*nodeIterator);
		const char* namePtr = node->GetAttribute(STRUCT_FIELD_NAME);
		const char* valuePtr = node->GetAttribute(STRUCT_FIELD_VALUE);
		if(namePtr == NULL || valuePtr == NULL) continue;
		string valueString(valuePtr);
		uint128 value;
		memset(&value, 0, sizeof(uint128));
		for(unsigned int i = 0; i < 4; i++)
		{
			if(valueString.length() == 0) break;
			int start = max<int>(0, static_cast<int>(valueString.length()) - 8);
			string subString(valueString.begin() + start, valueString.end());
			value.nV[i] = lexical_cast_hex<string>(subString);
			valueString = string(valueString.begin(), valueString.begin() + start);
		}
		m_registers[namePtr] = Register(4, value);
	}
}

void CStructFile::Write(Xml::CNode* rootNode) const
{
	for(const auto& registerIterator : m_registers)
	{
		const Register& reg(registerIterator.second);
		Xml::CNode* fieldNode = new Xml::CNode(STRUCT_FIELD, true);
		string valueString;
		for(unsigned int i = 0; i < reg.first; i++)
		{
			valueString = lexical_cast_hex<string>(reg.second.nV[i], 8) + valueString;
		}
		fieldNode->InsertAttribute(STRUCT_FIELD_NAME, registerIterator.first.c_str());
		fieldNode->InsertAttribute(STRUCT_FIELD_VALUE, valueString.c_str());
		rootNode->InsertNode(fieldNode);
	}
}

void CStructFile::SetRegister32(const char* name, uint32 value)
{
	uint128 longValue;
	longValue.nD0 = value;
	longValue.nD1 = 0;
	m_registers[name] = Register(1, longValue);
}

void CStructFile::SetRegister64(const char* name, uint64 value)
{
	uint128 longValue;
	longValue.nD0 = value;
	longValue.nD1 = 0;
	m_registers[name] = Register(2, longValue);
}

uint32 CStructFile::GetRegister32(const char* name) const
{
	RegisterList::const_iterator registerIterator(m_registers.find(name));
	if(registerIterator == m_registers.end()) return 0;
	return registerIterator->second.second.nV0;
}

uint64 CStructFile::GetRegister64(const char* name) const
{
	RegisterList::const_iterator registerIterator(m_registers.find(name));
	if(registerIterator == m_registers.end()) return 0;
	return registerIterator->second.second.nD0;
}
