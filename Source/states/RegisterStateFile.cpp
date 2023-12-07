#include <string.h>
#include <memory>
#include "RegisterStateFile.h"
#include "xml/Node.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "lexical_cast_ex.h"

#define REGISTER_STATE_NODE "RegisterState"

CRegisterStateFile::CRegisterStateFile(const char* name)
    : CZipFile(name)
{
}

CRegisterStateFile::CRegisterStateFile(Framework::CStream& stream)
    : CZipFile("")
{
	Read(stream);
}

void CRegisterStateFile::Read(Framework::CStream& stream)
{
	auto rootNode = Framework::Xml::CParser::ParseDocument(stream);
	auto registerStateNode = rootNode->Select(REGISTER_STATE_NODE);
	if(registerStateNode)
	{
		m_registers.Read(registerStateNode);
	}
}

void CRegisterStateFile::Write(Framework::CStream& stream)
{
	auto rootNode = std::make_unique<Framework::Xml::CNode>(REGISTER_STATE_NODE, true);
	m_registers.Write(rootNode.get());
	Framework::Xml::CWriter::WriteDocument(stream, rootNode.get());
}

void CRegisterStateFile::SetRegister32(const char* name, uint32 value)
{
	m_registers.SetRegister32(name, value);
}

void CRegisterStateFile::SetRegister64(const char* name, uint64 value)
{
	m_registers.SetRegister64(name, value);
}

void CRegisterStateFile::SetRegister128(const char* name, uint128 value)
{
	m_registers.SetRegister128(name, value);
}

uint32 CRegisterStateFile::GetRegister32(const char* name) const
{
	return m_registers.GetRegister32(name);
}

uint64 CRegisterStateFile::GetRegister64(const char* name) const
{
	return m_registers.GetRegister64(name);
}

uint128 CRegisterStateFile::GetRegister128(const char* name) const
{
	return m_registers.GetRegister128(name);
}
