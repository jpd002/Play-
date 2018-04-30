#pragma once

#include "Types.h"
#include "xml/Node.h"
#include <map>
#include <string>
#include <boost/signals2.hpp>

class CMIPSTags
{
public:
	typedef std::map<uint32, std::string> TagMap;
	typedef TagMap::const_iterator TagIterator;

	boost::signals2::signal<void()> OnTagListChange;

	void InsertTag(uint32, const char*);
	void RemoveTags();
	const char* Find(uint32) const;

	void Serialize(Framework::Xml::CNode*, const char*) const;
	void Unserialize(Framework::Xml::CNode*, const char*);

	void Serialize(Framework::Xml::CNode*) const;
	void Unserialize(Framework::Xml::CNode*);

	void Serialize(const char*) const;
	void Unserialize(const char*);

	TagIterator GetTagsBegin() const;
	TagIterator GetTagsEnd() const;

private:
	TagMap m_tags;
};
