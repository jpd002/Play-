#include "CommentsView.h"

CCommentsView::CCommentsView(QMdiArea* parent)
    : CTagsView(parent)
{
	setWindowTitle("Comments");
	{
		Strings strings;
		strings.newTagString = tr("New Comment");
		strings.renameTagString = tr("Rename Comment");
		strings.tagNameString = tr("New Comment Name:");
		strings.tagAddressString = tr("New Comment Address:");
		strings.deleteTagConfirmString = tr("Are you sure you want to delete this comment?");
		strings.deleteModuleTagsConfirmString = tr("Are you sure you want to delete comments from module '%0'?");
		SetStrings(strings);
	}
}

void CCommentsView::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	CTagsView::SetContext(context, &context->m_Comments, biosDebugInfoProvider);
}
