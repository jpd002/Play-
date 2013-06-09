#pragma once

#include "../RegViewPage.h"
#include "../../GSHandler.h"
#include "FrameDebuggerTab.h"

class CGsInputStateView : public CRegViewPage, public IFrameDebuggerTab
{
public:
							CGsInputStateView(HWND, const RECT&);
	virtual					~CGsInputStateView();

	void					UpdateState(CGSHandler*) override;
};
