#include <string.h>
#include <memory>
#include "RegisterStateFile.h"
#include "xml/Node.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "lexical_cast_ex.h"

CRegisterStateFile::CRegisterStateFile(const char* name)
    : CZipFile(name)
{
}

CRegisterStateFile::CRegisterStateFile(Framework::CStream& stream)
    : CZipFile("")
{
	Read(stream);
}

CRegisterStateFile::~CRegisterStateFile()
{
}

void CRegisterStateFile::Read(Framework::CStream& stream)
{
	m_registers.clear();
	auto rootNode = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(stream));
	auto registerList = rootNode->SelectNodes("RegisterFile/Register");
	for(Framework::Xml::CNode::NodeIterator nodeIterator(registerList.begin());
	    nodeIterator != registerList.end(); nodeIterator++)
	{
		try
		{
			auto node(*nodeIterator);
			const char* namePtr = node->GetAttribute("Name");
			const char* valuePtr = node->GetAttribute("Value");
			if(namePtr == NULL || valuePtr == NULL) continue;
			std::string valueString(valuePtr);
			uint128 value;
			memset(&value, 0, sizeof(uint128));
			for(unsigned int i = 0; i < 4; i++)
			{
				if(valueString.length() == 0) break;
				int start = std::max<int>(0, static_cast<int>(valueString.length()) - 8);
				std::string subString(valueString.begin() + start, valueString.end());
				value.nV[i] = lexical_cast_hex<std::string>(subString);
				valueString = std::string(valueString.begin(), valueString.begin() + start);
			}
			m_registers[namePtr] = Register(4, value);
		}
		catch(...)
		{
		}
	}
}

void CRegisterStateFile::Write(Framework::CStream& stream)
{
	auto rootNode = new Framework::Xml::CNode("RegisterFile", true);
	for(auto registerIterator(m_registers.begin());
	    registerIterator != m_registers.end(); registerIterator++)
	{
		const Register& reg(registerIterator->second);
		auto registerNode = new Framework::Xml::CNode("Register", true);
		std::string valueString;
		for(unsigned int i = 0; i < reg.first; i++)
		{
			valueString = lexical_cast_hex<std::string>(reg.second.nV[i], 8) + valueString;
		}
		registerNode->InsertAttribute("Name", registerIterator->first.c_str());
		registerNode->InsertAttribute("Value", valueString.c_str());
		rootNode->InsertNode(registerNode);
	}
	Framework::Xml::CWriter::WriteDocument(stream, rootNode);
	delete rootNode;
}

void CRegisterStateFile::SetRegister32(const char* name, uint32 value)
{
	uint128 longValue;
	longValue.nD0 = value;
	longValue.nD1 = 0;
	m_registers[name] = Register(1, longValue);
}

void CRegisterStateFile::SetRegister64(const char* name, uint64 value)
{
	uint128 longValue;
	longValue.nD0 = value;
	longValue.nD1 = 0;
	m_registers[name] = Register(2, longValue);
}

void CRegisterStateFile::SetRegister128(const char* name, uint128 value)
{
	m_registers[name] = Register(4, value);
}

uint32 CRegisterStateFile::GetRegister32(const char* name) const
{
	auto registerIterator(m_registers.find(name));
	if(registerIterator == m_registers.end()) return 0;
	return registerIterator->second.second.nV0;
}

uint64 CRegisterStateFile::GetRegister64(const char* name) const
{
	auto registerIterator(m_registers.find(name));
	if(registerIterator == m_registers.end()) return 0;
	return registerIterator->second.second.nD0;
}

uint128 CRegisterStateFile::GetRegister128(const char* name) const
{
	auto registerIterator(m_registers.find(name));
	if(registerIterator == m_registers.end())
	{
		uint128 zero = {};
		return zero;
	}
	return registerIterator->second.second;
}
