#pragma once

class CGSHandler;
class CGsPacketMetadata;
struct DRAWINGKICK_INFO;

class IFrameDebuggerTab
{
public:
	virtual				~IFrameDebuggerTab() {}
	virtual void		UpdateState(CGSHandler*, CGsPacketMetadata*, DRAWINGKICK_INFO*) = 0;
};
