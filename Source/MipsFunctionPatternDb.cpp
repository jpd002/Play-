#include <string.h>
#include "MipsFunctionPatternDb.h"
#include "xml/FilteringNodeIterator.h"

CMipsFunctionPatternDb::CMipsFunctionPatternDb(Framework::Xml::CNode* node)
{
	Read(node);
}

CMipsFunctionPatternDb::~CMipsFunctionPatternDb()
{

}

const CMipsFunctionPatternDb::PatternArray& CMipsFunctionPatternDb::GetPatterns() const
{
	return m_patterns;
}

void CMipsFunctionPatternDb::Read(Framework::Xml::CNode* node)
{
	auto patternsNode = node->Select("FunctionPatterns");
	if(!patternsNode) return;

	m_patterns.reserve(patternsNode->GetChildCount());

	for(Framework::Xml::CFilteringNodeIterator patternNodeIterator(patternsNode, "FunctionPattern"); 
		!patternNodeIterator.IsEnd(); patternNodeIterator++)
	{
		auto patternNode = (*patternNodeIterator);

		const char* patternName = patternNode->GetAttribute("Name");
		if(patternName == NULL) continue;

		const char* patternSource = patternNode->GetInnerText();
		
		Pattern pattern = ParsePattern(patternSource);
		pattern.name = patternName;
		m_patterns.push_back(pattern);
	}
}

CMipsFunctionPatternDb::Pattern CMipsFunctionPatternDb::ParsePattern(const char* source)
{
	Pattern result;
	result.items.reserve(0x40);

	bool insideComment = false;
	std::string currValue;
	while(1)
	{
		char currChar = (*source++);
		if(currChar == 0) break;
		if(insideComment)
		{
			if(currChar == '\n')
			{
				insideComment = false;
			}
			continue;
		}
		if(currChar == ';')
		{
			insideComment = true;
			continue;
		}
		if(isspace(currChar))
		{
			if(currValue.length() != 0)
			{
				//Parse value
				PATTERNITEM item = { 0 };
				if(ParsePatternItem(currValue.c_str(), item))
				{
					result.items.push_back(item);
				}
				currValue.clear();
			}
			continue;
		}
		currValue += currChar;
	}

	return result;
}

bool CMipsFunctionPatternDb::ParsePatternItem(const char* source, PATTERNITEM& result)
{
	size_t sourceLength = strlen(source);
	if(!strcmp(source, "XXXXXXXX"))
	{
		result.mask = 0;
		result.value = 0;
	}
	else if((sourceLength == 8) && !strcmp(source + 4, "XXXX"))
	{
		result.mask = 0xFFFF0000;
		result.value = 0;
		sscanf(source, "%x", &result.value);
		result.value <<= 16;
	}
	else
	{
		result.mask = 0xFFFFFFFF;
		result.value = 0;
		sscanf(source, "%x", &result.value);
		if(result.value == 0)
		{
			return false;
		}
	}
	return true;
}

bool CMipsFunctionPatternDb::Pattern::Matches(uint32* text, uint32 textSize) const
{
	textSize /= 4;
	if(textSize < items.size()) return false;

	unsigned int itemIndex = 0;
	unsigned int textIndex = 0;
	unsigned int matchCount = 0;
	while(1)
	{
		if(itemIndex == items.size()) break;
		if(textIndex == textSize) break;
		uint32 srcValue = text[textIndex++];
		if(textIndex != 1 && srcValue == 0) continue;
		const PATTERNITEM& item(items[itemIndex++]);
		srcValue &= item.mask;
		if(srcValue != item.value) return false;
		matchCount++;
	}

	return (matchCount == items.size());
}
