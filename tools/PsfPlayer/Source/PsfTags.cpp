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

bool CPsfTags::HasTag(const char* tagName) const
{
	ConstTagIterator tagIterator = m_tags.find(tagName);
	return tagIterator != m_tags.end();
}

string CPsfTags::GetRawTagValue(const char* tagName) const
{
	ConstTagIterator tagIterator = m_tags.find(tagName);
	if(tagIterator == m_tags.end()) return "";
	return tagIterator->second;
}

wstring CPsfTags::GetTagValue(const char* tagName) const
{
	return m_stringConverter(GetRawTagValue(tagName));
}

wstring CPsfTags::DecodeTagValue(const char* value) const
{
	return m_stringConverter(string(value));
}

double CPsfTags::ConvertTimeString(const wchar_t* value)
{
    double time = 0;

    enum
    {
        STATE_CHECK_HAS_SEPARATOR,
        STATE_CHECK_HAS_OTHER_SEPARATOR,
        STATE_SPLIT_SECONDS_DECIMAL,
        STATE_DONE
    };

    struct FindSeparator
    {
        const wchar_t* operator()(const wchar_t* string)
        {
            const wchar_t* separator = wcschr(string, ':');
            if(separator == NULL)
            {
                separator = wcschr(string, ';');
            }
            return separator;
        }
    };

    unsigned int currentState = STATE_CHECK_HAS_SEPARATOR;
    std::wstring firstUnit;

    while(currentState != STATE_DONE)
    {
        if(currentState == STATE_CHECK_HAS_SEPARATOR)
        {
            const wchar_t* separator = FindSeparator()(value);
            if(separator == NULL)
            {
                //No colon found -> doesn't have hours or minutes
                currentState = STATE_SPLIT_SECONDS_DECIMAL;
                continue;
            }

            firstUnit = std::wstring(value, separator);
            currentState = STATE_CHECK_HAS_OTHER_SEPARATOR;
            value = separator + 1;
        }
        else if(currentState == STATE_CHECK_HAS_OTHER_SEPARATOR)
        {
            const wchar_t* separator = FindSeparator()(value);
            std::wstring minutesStr, hoursStr;
            if(separator == NULL)
            {
                //No colon found -> first unit is minutes
                minutesStr = firstUnit;
            }
            else
            {
                hoursStr = firstUnit;
                minutesStr = std::wstring(value, separator);
                value = separator + 1;
            }
            int hours = wcstol(hoursStr.c_str(), NULL, 10);
            int minutes = wcstol(minutesStr.c_str(), NULL, 10);
            currentState = STATE_SPLIT_SECONDS_DECIMAL;
            time += minutes * 60;
            time += hours * 60 * 60;
        }
        else if(currentState == STATE_SPLIT_SECONDS_DECIMAL)
        {
            double seconds = wcstod(value, NULL);
            time += seconds;
            currentState = STATE_DONE;
        }
    }

    return time;
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
