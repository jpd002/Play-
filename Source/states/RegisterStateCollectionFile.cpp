#include "RegisterStateCollectionFile.h"
#include <memory>
#include "xml/Node.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "lexical_cast_ex.h"

#define COLLECTION_NODE "RegisterStates"
#define COLLECTION_REGISTERSTATE_NODE "RegisterState"
#define COLLECTION_REGISTERSTATE_ATTR_NAME "Name"
#define COLLECTION_REGISTERSTATES_PATH (COLLECTION_NODE "/" COLLECTION_REGISTERSTATE_NODE)

CRegisterStateCollectionFile::CRegisterStateCollectionFile(const char* name)
    : CZipFile(name)
{
}

CRegisterStateCollectionFile::CRegisterStateCollectionFile(Framework::CStream& stream)
    : CZipFile("")
{
	Read(stream);
}

CRegisterStateCollectionFile::RegisterStateIterator CRegisterStateCollectionFile::begin() const
{
	return std::begin(m_registerStates);
}

CRegisterStateCollectionFile::RegisterStateIterator CRegisterStateCollectionFile::end() const
{
	return std::end(m_registerStates);
}

const CRegisterState& CRegisterStateCollectionFile::GetRegisterState(const char* name)
{
	assert(m_registerStates.find(name) != std::end(m_registerStates));
	return m_registerStates[name];
}

void CRegisterStateCollectionFile::InsertRegisterState(const char* name, CRegisterState registerState)
{
	m_registerStates[name] = std::move(registerState);
}

void CRegisterStateCollectionFile::Read(Framework::CStream& stream)
{
	m_registerStates.clear();
	auto rootNode = Framework::Xml::CParser::ParseDocument(stream);
	auto registerStateList = rootNode->SelectNodes(COLLECTION_REGISTERSTATES_PATH);
	for(auto* node : registerStateList)
	{
		try
		{
			const char* namePtr = node->GetAttribute(COLLECTION_REGISTERSTATE_ATTR_NAME);
			if(!namePtr) continue;
			CRegisterState registerState;
			registerState.Read(node);
			m_registerStates[namePtr] = std::move(registerState);
		}
		catch(...)
		{
		}
	}
}

void CRegisterStateCollectionFile::Write(Framework::CStream& stream)
{
	auto rootNode = std::make_unique<Framework::Xml::CNode>(COLLECTION_NODE, true);
	for(const auto& registerStatePair : m_registerStates)
	{
		const auto& structFile(registerStatePair.second);
		auto structNode = std::make_unique<Framework::Xml::CNode>(COLLECTION_REGISTERSTATE_NODE, true);
		structNode->InsertAttribute(std::make_pair(COLLECTION_REGISTERSTATE_ATTR_NAME, registerStatePair.first));
		structFile.Write(structNode.get());
		rootNode->InsertNode(std::move(structNode));
	}
	Framework::Xml::CWriter::WriteDocument(stream, rootNode.get());
}
