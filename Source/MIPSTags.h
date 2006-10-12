#ifndef _MIPSTAGS_H_
#define _MIPSTAGS_H_

#include "Types.h"
#include <map>

class CMIPSTags
{
public:
	typedef std::map<uint32, std::string>	TagMap;
	typedef TagMap::const_iterator			TagIterator;

								CMIPSTags();
								~CMIPSTags();
	void						InsertTag(uint32, const char*);
	void						RemoveTags();
	const char*					Find(uint32);
	void						Serialize(const char*);
	void						Unserialize(const char*);

	TagIterator					GetTagsBegin() const;
	TagIterator					GetTagsEnd() const;

private:

	TagMap						m_Tags;
};

#endif
