#include <algorithm>
#include "PsfTagsDescription.h"

PsfTags::TagList PsfTags::ParseTags(Framework::Xml::CNode* tagsNode)
{
	TagList result;

	for(Framework::Xml::CNode::NodeIterator nodeIterator(tagsNode->GetChildrenBegin());
		nodeIterator != tagsNode->GetChildrenEnd(); nodeIterator++)
	{
		Framework::Xml::CNode* node = (*nodeIterator);
		if(!node->IsTag()) continue;
		std::string tagName = node->GetText();
		std::string tagValue = node->GetInnerText();

		std::transform(tagName.begin(), tagName.end(), tagName.begin(), tolower);
		result[tagName] = tagValue;
	}

	return result;
}
