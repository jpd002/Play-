#include "MIPSTags.h"
#include "PtrMacro.h"
#include "StdStream.h"
#include "lexical_cast_ex.h"
#include "xml/FilteringNodeIterator.h"

#define TAG_ELEMENT_NAME				("tag")
#define TAG_ELEMENT_ATTRIBUTE_ADDRESS	("address")
#define TAG_ELEMENT_ATTRIBUTE_VALUE		("value")

using namespace Framework;
using namespace std;

CMIPSTags::CMIPSTags()
{

}

CMIPSTags::~CMIPSTags()
{
	RemoveTags();
}

void CMIPSTags::InsertTag(uint32 nAddress, const char* sTag)
{
	bool nErase = false;
	if(sTag == NULL)
	{
		nErase = true;
	}
	else
	{
		nErase = (strlen(sTag) == 0);
	}

	if(nErase)
	{
		TagMap::iterator itTag(m_Tags.find(nAddress));

		if(itTag != m_Tags.end())
		{
			m_Tags.erase(itTag);
		}
	}
	else
	{
		m_Tags[nAddress] = sTag;
	}
}

void CMIPSTags::RemoveTags()
{
	m_Tags.clear();
}

const char* CMIPSTags::Find(uint32 nAddress)
{
	TagMap::const_iterator itTag(m_Tags.find(nAddress));
	return (itTag != m_Tags.end()) ? (itTag->second.c_str()) : (NULL);
}

void CMIPSTags::Serialize(const char* sPath)
{
	CStdStream Stream(fopen(sPath, "wb"));

	Stream.Write32(static_cast<uint32>(m_Tags.size()));

	for(TagMap::const_iterator itTag(m_Tags.begin());
		itTag != m_Tags.end(); itTag++)
	{
		const string& sTag = itTag->second;

		uint8 nLength = static_cast<uint8>(min<size_t>(sTag.length(), 255));

		Stream.Write32(itTag->first);
		Stream.Write8(nLength);
		Stream.Write(sTag.c_str(), nLength);
	}
}

void CMIPSTags::Unserialize(const char* sPath)
{
	try
	{
		CStdStream Stream(fopen(sPath, "rb"));
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

void CMIPSTags::Serialize(Xml::CNode* parentNode, const char* sectionName)
{
    Xml::CNode* section = new Xml::CNode(sectionName, true);
    Serialize(section);
    parentNode->InsertNode(section);
}

void CMIPSTags::Unserialize(Xml::CNode* parentNode, const char* sectionName)
{
    Xml::CNode* section = parentNode->Select(sectionName);
    if(!section) return;
    Unserialize(section);
}

void CMIPSTags::Serialize(Xml::CNode* parentNode)
{
	for(TagMap::const_iterator itTag(m_Tags.begin());
		itTag != m_Tags.end(); itTag++)
	{
        Xml::CNode* node = new Xml::CNode(TAG_ELEMENT_NAME, true);
        node->InsertAttribute(TAG_ELEMENT_ATTRIBUTE_ADDRESS, lexical_cast_hex<string>(itTag->first, 8).c_str());
        node->InsertAttribute(TAG_ELEMENT_ATTRIBUTE_VALUE, itTag->second.c_str());
        parentNode->InsertNode(node);
	}
}

void CMIPSTags::Unserialize(Xml::CNode* parentNode)
{
	for(Xml::CFilteringNodeIterator nodeIterator(parentNode, TAG_ELEMENT_NAME);
		!nodeIterator.IsEnd(); nodeIterator++)
	{
		Xml::CNode* node = *nodeIterator;
		const char* addressText = node->GetAttribute(TAG_ELEMENT_ATTRIBUTE_ADDRESS);
		const char* valueText	= node->GetAttribute(TAG_ELEMENT_ATTRIBUTE_VALUE);
		if(!addressText || !valueText) continue;
		uint32 address = lexical_cast_hex<string>(addressText);
		InsertTag(address, valueText);
	}
}

CMIPSTags::TagIterator CMIPSTags::GetTagsBegin() const
{
	return m_Tags.begin();
}

CMIPSTags::TagIterator CMIPSTags::GetTagsEnd() const
{
	return m_Tags.end();
}
