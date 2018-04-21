#pragma once

#include "../MIPS.h"
#include "../VirtualMachine.h"
#include "RegViewPage.h"

class CRegViewSCU : public CRegViewPage, public boost::signals2::trackable
{
public:
	CRegViewSCU(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual ~CRegViewSCU() = default;

private:
	void        Update() override;
	std::string GetDisplayText();

	CMIPS* m_ctx = nullptr;
};
