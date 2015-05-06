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
	virtual										~CPixelBufferViewOverlay();

protected:
	long										OnCommand(unsigned short, unsigned short, HWND) override;

private:
	std::unique_ptr<Framework::Win32::CButton>	m_saveButton;
	std::unique_ptr<Framework::Win32::CButton>	m_fitButton;
};
