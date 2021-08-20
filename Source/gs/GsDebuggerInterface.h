#pragma once

#include "GSHandler.h"

class CGsDebuggerInterface
{
public:
	virtual bool GetDepthTestingEnabled() const = 0;
	virtual void SetDepthTestingEnabled(bool) = 0;

	virtual bool GetAlphaBlendingEnabled() const = 0;
	virtual void SetAlphaBlendingEnabled(bool) = 0;

	virtual bool GetAlphaTestingEnabled() const = 0;
	virtual void SetAlphaTestingEnabled(bool) = 0;

	virtual Framework::CBitmap GetFramebuffer(uint64) = 0;
	virtual Framework::CBitmap GetTexture(uint64, uint32, uint64, uint64, uint32) = 0;

	virtual int GetFramebufferScale() = 0;

	virtual const CGSHandler::VERTEX* GetInputVertices() const = 0;
};
