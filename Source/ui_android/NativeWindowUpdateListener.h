#pragma once

class ANativeWindow;

class INativeWindowUpdateListener
{
public:
	virtual void SetWindow(ANativeWindow*) = 0;
};
