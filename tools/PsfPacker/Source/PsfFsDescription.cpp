#include "PsfFsDescription.h"
#include "xml/Utils.h"

PsfFs::FilePtr PsfFs::ParseFileNode(Framework::Xml::CNode* fileNode)
{
	FilePtr result(new FILE());
	result->source = Framework::Xml::GetAttributeStringValue(fileNode, "Source");
	return result;
}

PsfFs::DirectoryPtr PsfFs::ParseDirectoryNode(Framework::Xml::CNode* directoryNode)
{
	DirectoryPtr result(new DIRECTORY());
	for(Framework::Xml::CNode::NodeIterator nodeIterator(directoryNode->GetChildrenBegin());
		nodeIterator != directoryNode->GetChildrenEnd(); nodeIterator++)
	{
		Framework::Xml::CNode* node(*nodeIterator);
		if(!node->IsTag()) continue;
		NodePtr newChild;
		if(!strcmp(node->GetText(), "Directory"))
		{
			newChild = ParseDirectoryNode(node);
		}
		if(!strcmp(node->GetText(), "File"))
		{
			newChild = ParseFileNode(node);
		}
		newChild->name = Framework::Xml::GetAttributeStringValue(node, "Name");
		result->nodes.push_back(newChild);
	}
	return result;
}
