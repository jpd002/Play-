#include "RegisterState.h"
#include <cstring>
#include "xml/Node.h"
#include "lexical_cast_ex.h"

#define NODE_REGISTER ("Register")
#define NODE_REGISTER_ATTR_NAME ("Name")
#define NODE_REGISTER_ATTR_VALUE ("Value")

using namespace Framework;
using namespace std;

void CRegisterState::Read(Xml::CNode* rootNode)
{
	m_registers.clear();
	auto registerList = rootNode->SelectNodes(NODE_REGISTER);
	for(auto* node : registerList)
	{
		try
		{
			const char* namePtr = node->GetAttribute(NODE_REGISTER_ATTR_NAME);
			const char* valuePtr = node->GetAttribute(NODE_REGISTER_ATTR_VALUE);
			if(!namePtr || !valuePtr) continue;
			std::string valueString(valuePtr);
			uint128 value = {};
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

void CRegisterState::Write(Xml::CNode* rootNode) const
{
	for(auto registerIterator(m_registers.begin());
	    registerIterator != m_registers.end(); registerIterator++)
	{
		const Register& reg(registerIterator->second);
		auto registerNode = std::make_unique<Framework::Xml::CNode>(NODE_REGISTER, true);
		std::string valueString;
		for(unsigned int i = 0; i < reg.first; i++)
		{
			valueString = lexical_cast_hex<std::string>(reg.second.nV[i], 8) + valueString;
		}
		registerNode->InsertAttribute(std::make_pair(NODE_REGISTER_ATTR_NAME, registerIterator->first));
		registerNode->InsertAttribute(std::make_pair(NODE_REGISTER_ATTR_VALUE, std::move(valueString)));
		rootNode->InsertNode(std::move(registerNode));
	}
}

void CRegisterState::SetRegister32(const char* name, uint32 value)
{
	uint128 longValue;
	longValue.nD0 = value;
	longValue.nD1 = 0;
	m_registers[name] = Register(1, longValue);
}

void CRegisterState::SetRegister64(const char* name, uint64 value)
{
	uint128 longValue;
	longValue.nD0 = value;
	longValue.nD1 = 0;
	m_registers[name] = Register(2, longValue);
}

void CRegisterState::SetRegister128(const char* name, uint128 value)
{
	m_registers[name] = Register(4, value);
}

uint32 CRegisterState::GetRegister32(const char* name) const
{
	auto registerIterator(m_registers.find(name));
	if(registerIterator == m_registers.end()) return 0;
	return registerIterator->second.second.nV0;
}

uint64 CRegisterState::GetRegister64(const char* name) const
{
	auto registerIterator(m_registers.find(name));
	if(registerIterator == m_registers.end()) return 0;
	return registerIterator->second.second.nD0;
}

uint128 CRegisterState::GetRegister128(const char* name) const
{
	auto registerIterator(m_registers.find(name));
	if(registerIterator == m_registers.end())
	{
		uint128 zero = {};
		return zero;
	}
	return registerIterator->second.second;
}
