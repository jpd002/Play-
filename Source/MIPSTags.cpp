#include "MIPSTags.h"
#include "PtrMacro.h"
#include "StdStream.h"

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
	TagMap::iterator itTag;

	//Check if it already exists
	itTag = m_Tags.find(nAddress);
	if(itTag == m_Tags.end())
	{
		m_Tags[nAddress] = sTag;
	}
	else
	{
		m_Tags.erase(itTag);
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
		uint8 nLength;

		nLength = static_cast<uint8>(min<size_t>(sTag.length(), 255));

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
		uint32 nCount;

		RemoveTags();

		nCount = Stream.Read32();

		for(uint32 i = 0; i < nCount; i++)
		{
			uint32 nKey;
			uint8 nLength;
			char sTag[256];

			nKey		= Stream.Read32();
			nLength		= Stream.Read8();

			Stream.Read(sTag, nLength);
			sTag[nLength] = 0;

			InsertTag(nKey, sTag);
		}
	}
	catch(...)
	{
		
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
