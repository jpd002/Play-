#include "VariablesView.h"

CVariablesView::CVariablesView(QMdiArea* parent)
    : CTagsView(parent)
{
	setWindowTitle("Variables");
}

void CVariablesView::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	CTagsView::SetContext(context, &context->m_Variables, biosDebugInfoProvider);
}
