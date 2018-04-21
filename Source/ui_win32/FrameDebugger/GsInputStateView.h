#pragma once

#include "../../gs/GSHandler.h"
#include "../RegViewPage.h"
#include "FrameDebuggerTab.h"

class CGsInputStateView : public CRegViewPage, public IFrameDebuggerTab
{
public:
	CGsInputStateView(HWND, const RECT&);
	virtual ~CGsInputStateView();

	void UpdateState(CGSHandler*, CGsPacketMetadata*, DRAWINGKICK_INFO*) override;
};
