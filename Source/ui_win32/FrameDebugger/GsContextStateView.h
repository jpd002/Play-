#pragma once

#include "../../gs/GSHandler.h"
#include "../RegViewPage.h"

class CGsContextStateView : public CRegViewPage
{
public:
	CGsContextStateView(HWND, const RECT&, unsigned int);
	virtual ~CGsContextStateView();

	void UpdateState(CGSHandler*);

private:
	unsigned int m_contextId;
};
