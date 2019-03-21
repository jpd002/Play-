#pragma once

#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewSCU : public CRegViewPage
{
public:
	CRegViewSCU(HWND, const RECT&, CMIPS*);
	virtual ~CRegViewSCU() = default;

private:
	void Update() override;
	std::string GetDisplayText();

	CMIPS* m_ctx = nullptr;
};
