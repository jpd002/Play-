#include "MIPSTags.h"
#include "StdStream.h"
#include "lexical_cast_ex.h"
#include "xml/FilteringNodeIterator.h"

#define TAG_ELEMENT_NAME				("tag")
#define TAG_ELEMENT_ATTRIBUTE_ADDRESS	("address")
#define TAG_ELEMENT_ATTRIBUTE_VALUE		("value")

void CMIPSTags::InsertTag(uint32 nAddress, const char* sTag)
{
	bool nErase = false;
	if(sTag == nullptr)
	{
		nErase = true;
	}
	else
	{
		nErase = (strlen(sTag) == 0);
	}

	if(nErase)
	{
		auto tagIterator = m_tags.find(nAddress);
		if(tagIterator != m_tags.end())
		{
			m_tags.erase(tagIterator);
		}
	}
	else
	{
		m_tags[nAddress] = sTag;
	}
}

void CMIPSTags::RemoveTags()
{
	m_tags.clear();
}

const char* CMIPSTags::Find(uint32 nAddress) const
{
	auto tagIterator = m_tags.find(nAddress);
	return (tagIterator != m_tags.end()) ? (tagIterator->second.c_str()) : nullptr;
}

void CMIPSTags::Serialize(const char* sPath) const
{
	Framework::CStdStream stream(fopen(sPath, "wb"));

	stream.Write32(static_cast<uint32>(m_tags.size()));

	for(const auto& tagPair : m_tags)
	{
		const auto& sTag = tagPair.second;

		uint8 nLength = static_cast<uint8>(std::min<size_t>(sTag.length(), 255));

		stream.Write32(tagPair.first);
		stream.Write8(nLength);
		stream.Write(sTag.c_str(), nLength);
	}
}

void CMIPSTags::Unserialize(const char* sPath)
{
	try
	{
		Framework::CStdStream Stream(fopen(sPath, "rb"));
		RemoveTags();

		uint32 nCount = Stream.Read32();

		for(uint32 i = 0; i < nCount; i++)
		{
			char sTag[256];

			uint32 nKey		= Stream.Read32();
			uint8 nLength	= Stream.Read8();

			Stream.Read(sTag, nLength);
			sTag[nLength] = 0;

			InsertTag(nKey, sTag);
		}
	}
	catch(...)
	{
		
	}
}

void CMIPSTags::Serialize(Framework::Xml::CNode* parentNode, const char* sectionName) const
{
	auto section = new Framework::Xml::CNode(sectionName, true);
	Serialize(section);
	parentNode->InsertNode(section);
}

void CMIPSTags::Unserialize(Framework::Xml::CNode* parentNode, const char* sectionName)
{
	auto section = parentNode->Select(sectionName);
	if(!section) return;
	Unserialize(section);
}

void CMIPSTags::Serialize(Framework::Xml::CNode* parentNode) const
{
	for(const auto& tagPair : m_tags)
	{
		auto node = new Framework::Xml::CNode(TAG_ELEMENT_NAME, true);
		node->InsertAttribute(TAG_ELEMENT_ATTRIBUTE_ADDRESS, lexical_cast_hex<std::string>(tagPair.first, 8).c_str());
		node->InsertAttribute(TAG_ELEMENT_ATTRIBUTE_VALUE, tagPair.second.c_str());
		parentNode->InsertNode(node);
	}
}

void CMIPSTags::Unserialize(Framework::Xml::CNode* parentNode)
{
	for(Framework::Xml::CFilteringNodeIterator nodeIterator(parentNode, TAG_ELEMENT_NAME);
		!nodeIterator.IsEnd(); nodeIterator++)
	{
		auto node = *nodeIterator;
		auto addressText = node->GetAttribute(TAG_ELEMENT_ATTRIBUTE_ADDRESS);
		auto valueText   = node->GetAttribute(TAG_ELEMENT_ATTRIBUTE_VALUE);
		if(!addressText || !valueText) continue;
		uint32 address = lexical_cast_hex<std::string>(addressText);
		InsertTag(address, valueText);
	}
}

CMIPSTags::TagIterator CMIPSTags::GetTagsBegin() const
{
	return m_tags.begin();
}

CMIPSTags::TagIterator CMIPSTags::GetTagsEnd() const
{
	return m_tags.end();
}
