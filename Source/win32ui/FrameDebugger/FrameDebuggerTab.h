#pragma once

class CGSHandler;

class IFrameDebuggerTab
{
public:
	virtual				~IFrameDebuggerTab() {}
	virtual void		UpdateState(CGSHandler*) = 0;
};
