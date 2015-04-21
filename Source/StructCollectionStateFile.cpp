#include <memory>
#include "StructCollectionStateFile.h"
#include "xml/Node.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "lexical_cast_ex.h"

#define STRUCT_DOCUMENT_HEADER		("StructCollection")
#define STRUCT_DOCUMENT_DETAIL		("Struct")
#define STRUCT_DOCUMENT_DETAIL_NAME	("Name")

CStructCollectionStateFile::CStructCollectionStateFile(const char* name)
: CZipFile(name)
{

}

CStructCollectionStateFile::CStructCollectionStateFile(Framework::CStream& stream)
: CZipFile("")
{
	Read(stream);
}

CStructCollectionStateFile::~CStructCollectionStateFile()
{

}

CStructCollectionStateFile::StructIterator CStructCollectionStateFile::GetStructBegin() const
{
	return m_structs.begin();
}

CStructCollectionStateFile::StructIterator CStructCollectionStateFile::GetStructEnd() const
{
	return m_structs.end();
}

void CStructCollectionStateFile::InsertStruct(const char* name, const CStructFile& structFile)
{
	m_structs[name] = structFile;
}

void CStructCollectionStateFile::Read(Framework::CStream& stream)
{
	m_structs.clear();
	auto rootNode = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(stream));
	auto registerList = rootNode->SelectNodes((std::string(STRUCT_DOCUMENT_HEADER) + "/" + std::string(STRUCT_DOCUMENT_DETAIL)).c_str());
	for(auto nodeIterator(registerList.begin());
		nodeIterator != registerList.end(); nodeIterator++)
	{
		try
		{
			auto node(*nodeIterator);
			const char* namePtr = node->GetAttribute(STRUCT_DOCUMENT_DETAIL_NAME);
			if(namePtr == NULL) continue;
			CStructFile structFile;
			structFile.Read(node);
			m_structs[namePtr] = structFile;
		}
		catch(...)
		{

		}
	}
}

void CStructCollectionStateFile::Write(Framework::CStream& stream)
{
	auto rootNode = new Framework::Xml::CNode(STRUCT_DOCUMENT_HEADER, true);
	for(auto structIterator(m_structs.begin());
		structIterator != m_structs.end(); structIterator++)
	{
		const auto& structFile(structIterator->second);
		auto structNode = new Framework::Xml::CNode(STRUCT_DOCUMENT_DETAIL, true);
		structNode->InsertAttribute(STRUCT_DOCUMENT_DETAIL_NAME, structIterator->first.c_str());
		structFile.Write(structNode);
		rootNode->InsertNode(structNode);
	}
	Framework::Xml::CWriter::WriteDocument(stream, rootNode);
	delete rootNode;
}
