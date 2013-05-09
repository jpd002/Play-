#pragma once

#include "win32/Window.h"
#include "win32/Splitter.h"
#include "../../GSHandler.h"
#include "PixelBufferView.h"
#include "GsContextStateView.h"

class CGsContextView : public Framework::Win32::CWindow
{
public:
													CGsContextView(HWND, const RECT&);
	virtual											~CGsContextView();

	void											UpdateState(CGSHandler*);

protected:
	long											OnSize(unsigned int, unsigned int, unsigned int) override;

private:
	std::unique_ptr<Framework::Win32::CSplitter>	m_mainSplitter;
	std::unique_ptr<CPixelBufferView>				m_textureView;
	std::unique_ptr<CGsContextStateView>			m_stateView;

	unsigned int									m_contextId;
};
