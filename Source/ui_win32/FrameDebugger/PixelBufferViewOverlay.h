#pragma once

#include "win32/Window.h"
#include "win32/Button.h"
#include <memory>

class CPixelBufferViewOverlay : public Framework::Win32::CWindow
{
public:
	enum COMMANDS
	{
		COMMAND_SAVE = 1,
		COMMAND_FIT = 2
	};

	           CPixelBufferViewOverlay(HWND);
	           CPixelBufferViewOverlay(const CPixelBufferViewOverlay&) = delete;
	virtual    ~CPixelBufferViewOverlay() = default;

	CPixelBufferViewOverlay&    operator =(const CPixelBufferViewOverlay&) = delete;

protected:
	long    OnCommand(unsigned short, unsigned short, HWND) override;

private:
	Framework::Win32::CButton      m_saveButton;
	Framework::Win32::CButton      m_fitButton;
};
