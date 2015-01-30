#include "GameTestSheet.h"
#include "xml/Parser.h"
#include "xml/Utils.h"

CGameTestSheet::CGameTestSheet()
{

}

CGameTestSheet::CGameTestSheet(Framework::CStream& stream)
{
	ParseSheet(stream);
}

CGameTestSheet::~CGameTestSheet()
{

}

CGameTestSheet::ENVIRONMENT CGameTestSheet::GetEnvironment(uint32 id) const
{
	auto environmentIterator = m_environments.find(id);
	if(environmentIterator == std::end(m_environments))
	{
		return ENVIRONMENT();
	}
	else
	{
		return environmentIterator->second;
	}
}

const CGameTestSheet::TestArray& CGameTestSheet::GetTests() const
{
	return m_tests;
}

void CGameTestSheet::ParseSheet(Framework::CStream& stream)
{
	auto document = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(stream));
	//Environments
	{
		auto environmentNodes = document->SelectNodes("Game/Environments/Environment");
		for(const auto& environmentNode : environmentNodes)
		{
			auto id = Framework::Xml::GetAttributeIntValue(environmentNode, "Id");
			EnvironmentActionArray actions;
			for(const auto& actionNode : environmentNode->GetChildren())
			{
				if(!actionNode->IsTag()) continue;
				auto actionType = actionNode->GetText();
				auto actionName = Framework::Xml::GetAttributeStringValue(actionNode, "Name");
				int actionSize = 0;
				Framework::Xml::GetAttributeIntValue(actionNode, "Size", &actionSize);
				ENVIRONMENT_ACTION action;
				action.name = actionName;
				action.size = actionSize;
				if(!strcmp(actionType, "Directory"))
				{
					action.type = ENVIRONMENT_ACTION_CREATE_DIRECTORY;
				}
				else if(!strcmp(actionType, "File"))
				{
					action.type = ENVIRONMENT_ACTION_CREATE_FILE;
				}
				actions.push_back(action);
			}
			assert(m_environments.find(id) == std::end(m_environments));
			m_environments.insert(std::make_pair(id, actions));
		}
	}
	//Tests
	{
		auto testNodes = document->SelectNodes("Game/Tests/Test");
		for(const auto& testNode : testNodes)
		{
			TEST test;
			test.query = Framework::Xml::GetAttributeStringValue(testNode, "Query");
			test.environmentId = Framework::Xml::GetAttributeIntValue(testNode, "EnvironmentId");
			test.maxEntries = Framework::Xml::GetAttributeIntValue(testNode, "MaxEntries");
			test.result = Framework::Xml::GetAttributeIntValue(testNode, "Result");
			Framework::Xml::GetAttributeStringValue(testNode, "CurrentDirectory", &test.currentDirectory);
			auto entryNodes = testNode->SelectNodes("Entry");
			for(const auto& entryNode : entryNodes)
			{
				auto entryName = Framework::Xml::GetAttributeStringValue(entryNode, "Name");
				test.entries.push_back(entryName);
			}
			m_tests.push_back(test);
		}
	}
}
