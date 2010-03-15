#ifndef _PSFFSDESCRIPTION_H_
#define _PSFFSDESCRIPTION_H_

#include <memory>
#include <list>
#include "xml/Node.h"

namespace PsfFs
{
	struct NODE
	{
		virtual			~NODE() {}
		std::string		name;
	};
	typedef std::tr1::shared_ptr<NODE> NodePtr;
	typedef std::list<NodePtr> NodeList;

	struct DIRECTORY : public NODE
	{
		NodeList		nodes;
	};
	typedef std::tr1::shared_ptr<DIRECTORY> DirectoryPtr;

	struct FILE : public NODE
	{
		std::string		source;
	};
	typedef std::tr1::shared_ptr<FILE> FilePtr;

	FilePtr ParseFileNode(Framework::Xml::CNode*);
	DirectoryPtr ParseDirectoryNode(Framework::Xml::CNode*);
}

#endif
