#include "PsfTagsWriter.h"

using namespace PsfTags;

void CWriter::Write(Framework::CStream& output, const TagList& tags)
{
	if(tags.size() == 0) return;

	const char* tagHeader = "[TAG]";
	output.Write(tagHeader, strlen(tagHeader));

	for(TagList::const_iterator tagIterator(tags.begin()); 
		tagIterator != tags.end(); tagIterator++)
	{
		const std::string& tagName(tagIterator->first);
		const std::string& tagValue(tagIterator->second);
		output.Write(tagName.c_str(), tagName.length());
		output.Write8('=');
		output.Write(tagValue.c_str(), tagValue.length());
		output.Write8(0x0A);
	}
}
