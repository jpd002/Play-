#pragma once

class CGSHandler;
class CGsPacketMetadata;

class IFrameDebuggerTab
{
public:
	virtual				~IFrameDebuggerTab() {}
	virtual void		UpdateState(CGSHandler*, CGsPacketMetadata*) = 0;
};
