#include "PsfTags.h"
#include "string_cast_sjis.h"
#include "Utf8.h"

using namespace Framework;
using namespace std;

CPsfTags::CPsfTags()
{
	Init();
}

CPsfTags::CPsfTags(const TagMap& tags) :
m_tags(tags)
{
	Init();
}

CPsfTags::~CPsfTags()
{

}

void CPsfTags::Init()
{
	SetDefaultCharEncoding(SHIFT_JIS);
	SetStringConverter(m_defaultEncoding);
}

void CPsfTags::SetDefaultCharEncoding(const CHAR_ENCODING& encoding)
{
	m_defaultEncoding = encoding;
}

bool CPsfTags::HasTag(const char* tagName)
{
	ConstTagIterator tagIterator = m_tags.find(tagName);
	return tagIterator != m_tags.end();
}

string CPsfTags::GetRawTagValue(const char* tagName)
{
	ConstTagIterator tagIterator = m_tags.find(tagName);
	if(tagIterator == m_tags.end()) return "";
	return tagIterator->second;
}

wstring CPsfTags::GetTagValue(const char* tagName)
{
	return m_stringConverter(GetRawTagValue(tagName));
}

wstring CPsfTags::DecodeTagValue(const char* value)
{
	return m_stringConverter(string(value));
}

CPsfTags::ConstTagIterator CPsfTags::GetTagsBegin() const
{
	return m_tags.begin();
}

CPsfTags::ConstTagIterator CPsfTags::GetTagsEnd() const
{
	return m_tags.end();
}

void CPsfTags::SetStringConverter(const CHAR_ENCODING& encoding)
{
	switch(encoding)
	{
	case SHIFT_JIS:
		m_stringConverter = string_cast_sjis;
		break;
	case UTF8:
		m_stringConverter = Utf8::ConvertFromSafe;
		break;
	default:
		m_stringConverter = StringConvertFunction();
		break;
	}
}
