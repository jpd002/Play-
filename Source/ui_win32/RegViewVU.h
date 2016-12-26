#pragma once

#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewVU : public CRegViewPage, public boost::signals2::trackable
{
public:
									CRegViewVU(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewVU() = default;

	void							Update() override;

protected:
	long							OnRightButtonUp(int, int) override;
	long							OnCommand(unsigned short, unsigned short, HWND) override;

private:
	enum VIEWMODE
	{
		VIEWMODE_WORD,
		VIEWMODE_SINGLE,
		VIEWMODE_MAX,
	};

	std::string						GetDisplayText();

	CMIPS*							m_ctx = nullptr;
	VIEWMODE						m_viewMode = VIEWMODE_SINGLE;
};
