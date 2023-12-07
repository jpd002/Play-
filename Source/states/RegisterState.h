#pragma once

#include <map>
#include "xml/Node.h"
#include "uint128.h"

//NOTE: Making this inheritable prevents moving the object.
class CRegisterState final
{
public:
	void Read(Framework::Xml::CNode*);
	void Write(Framework::Xml::CNode*) const;

	void SetRegister32(const char*, uint32);
	void SetRegister64(const char*, uint64);
	void SetRegister128(const char*, uint128);

	uint32 GetRegister32(const char*) const;
	uint64 GetRegister64(const char*) const;
	uint128 GetRegister128(const char*) const;

private:
	typedef std::pair<uint8, uint128> Register;
	typedef std::map<std::string, Register> RegisterList;

	RegisterList m_registers;
};
