#include "GsInputStateView.h"
#include "GsStateUtils.h"

CGsInputStateView::CGsInputStateView(HWND parent, const RECT& rect)
: CRegViewPage(parent, rect)
{

}

CGsInputStateView::~CGsInputStateView()
{

}

void CGsInputStateView::UpdateState(CGSHandler* gs, CGsPacketMetadata*, DRAWINGKICK_INFO*)
{
	std::string result;
	result += CGsStateUtils::GetInputState(gs);
	SetDisplayText(result.c_str());
	CRegViewPage::Update();
}
