#pragma once

#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewVU : public CRegViewPage, public boost::signals2::trackable
{
public:
									CRegViewVU(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CRegViewVU();

	void							Update() override;

private:
	std::string						GetDisplayText();

	CMIPS*							m_ctx = nullptr;
};
