#pragma once

#include "../RegViewPage.h"
#include "../../GSHandler.h"

class CGsInputStateView : public CRegViewPage
{
public:
							CGsInputStateView(HWND, const RECT&);
	virtual					~CGsInputStateView();

	void					UpdateState(CGSHandler*);
};
