#pragma once

#include "win32/Window.h"
#include "win32/Button.h"
#include "win32/ComboBox.h"
#include <memory>
#include <vector>

class CPixelBufferViewOverlay : public Framework::Win32::CWindow
{
public:
	typedef std::vector<std::string> StringList;

	enum COMMANDS
	{
		COMMAND_SAVE = 1,
		COMMAND_FIT = 2,
		COMMAND_PIXELBUFFER_CHANGED = 3
	};

	           CPixelBufferViewOverlay(HWND);
	           CPixelBufferViewOverlay(const CPixelBufferViewOverlay&) = delete;
	virtual    ~CPixelBufferViewOverlay() = default;

	CPixelBufferViewOverlay&    operator =(const CPixelBufferViewOverlay&) = delete;

	void    SetPixelBufferTitles(StringList);

	int     GetSelectedPixelBufferIndex();
	void    SetSelectedPixelBufferIndex(int);

protected:
	long    OnCommand(unsigned short, unsigned short, HWND) override;

private:
	Framework::Win32::CButton      m_saveButton;
	Framework::Win32::CButton      m_fitButton;
	Framework::Win32::CComboBox    m_pixelBufferComboBox;
};
