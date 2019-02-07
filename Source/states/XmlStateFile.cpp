#include "XmlStateFile.h"
#include "xml/Parser.h"
#include "xml/Writer.h"

CXmlStateFile::CXmlStateFile(const char* name, const char* rootName)
    : CZipFile(name)
{
	m_root = std::make_unique<Framework::Xml::CNode>(rootName, true);
}

CXmlStateFile::CXmlStateFile(Framework::CStream& stream)
    : CZipFile("")
{
	Read(stream);
}

Framework::Xml::CNode* CXmlStateFile::GetRoot() const
{
	return m_root.get();
}

void CXmlStateFile::Read(Framework::CStream& stream)
{
	m_root = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(stream));
}

void CXmlStateFile::Write(Framework::CStream& stream)
{
	Framework::Xml::CWriter::WriteDocument(stream, m_root.get());
}
