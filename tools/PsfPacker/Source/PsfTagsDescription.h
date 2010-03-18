#ifndef _PSFTAGSDESCRIPTION_H_
#define _PSFTAGSDESCRIPTION_H_

#include <map>
#include "xml/Node.h"

namespace PsfTags
{
	typedef std::map<std::string, std::string> TagList;

	TagList ParseTags(Framework::Xml::CNode*);
}

#endif
