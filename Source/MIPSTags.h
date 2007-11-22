#ifndef _MIPSTAGS_H_
#define _MIPSTAGS_H_

#include "Types.h"
#include <map>
#include <string>
#include <boost/signal.hpp>

class CMIPSTags
{
public:
	typedef std::map<uint32, std::string>	TagMap;
	typedef TagMap::const_iterator			TagIterator;
    typedef boost::function<void ()>        TagListChangedHandler;

    boost::signal<void ()>                  m_OnTagListChanged;

								            CMIPSTags();
								            ~CMIPSTags();
	void						            InsertTag(uint32, const char*);
	void						            RemoveTags();
	const char*					            Find(uint32);
	void						            Serialize(const char*);
	void						            Unserialize(const char*);

	TagIterator					            GetTagsBegin() const;
	TagIterator					            GetTagsEnd() const;

private:

	TagMap						            m_Tags;
};

#endif
