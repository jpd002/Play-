#include "VariablesView.h"

CVariablesView::CVariablesView(QMdiArea* parent)
    : CTagsView(parent)
{
	setWindowTitle("Variables");
	{
		Strings strings;
		strings.newTagString = tr("New Variable");
		strings.renameTagString = tr("Rename Variable");
		strings.tagNameString = tr("New Variable Name:");
		strings.tagAddressString = tr("New Variable Address:");
		strings.deleteTagConfirmString = tr("Are you sure you want to delete this variable?");
		strings.deleteModuleTagsConfirmString = tr("Are you sure you want to delete variables from module '%0'?");
		SetStrings(strings);
	}
}

void CVariablesView::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	CTagsView::SetContext(context, &context->m_Variables, biosDebugInfoProvider);
}
