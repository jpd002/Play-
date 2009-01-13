#ifndef _PSFTAGMANAGER_H_
#define _PSFTAGMANAGER_H_

#include <functional>
#include "PsfBase.h"

class CPsfTags
{
public:
	typedef std::tr1::function<std::wstring (const std::string&)> StringConvertFunction;
	typedef CPsfBase::TagMap TagMap;
	typedef CPsfBase::ConstTagIterator ConstTagIterator;

	enum CHAR_ENCODING
	{
		INVALID,
		SHIFT_JIS,
		UTF8,
	};

							CPsfTags();
							CPsfTags(const TagMap&);
	virtual					~CPsfTags();
	void					SetDefaultCharEncoding(const CHAR_ENCODING&);
	bool					HasTag(const char*);
	std::string				GetRawTagValue(const char*);
	std::wstring			GetTagValue(const char*);
	std::wstring			DecodeTagValue(const char*);

	ConstTagIterator		GetTagsBegin() const;
	ConstTagIterator		GetTagsEnd() const;

private:
	void					Init();
	void					SetStringConverter(const CHAR_ENCODING&);

	CHAR_ENCODING			m_defaultEncoding;
	StringConvertFunction	m_stringConverter;
	TagMap					m_tags;
};

#endif
