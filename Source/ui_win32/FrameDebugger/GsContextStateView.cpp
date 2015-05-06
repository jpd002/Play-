#include "GsContextStateView.h"
#include "GsStateUtils.h"

CGsContextStateView::CGsContextStateView(HWND parent, const RECT& rect, unsigned int contextId)
: CRegViewPage(parent, rect)
, m_contextId(contextId)
{

}

CGsContextStateView::~CGsContextStateView()
{

}

void CGsContextStateView::UpdateState(CGSHandler* gs)
{
	std::string result;
	result += CGsStateUtils::GetContextState(gs, m_contextId);
	SetDisplayText(result.c_str());
	CRegViewPage::Update();
}
