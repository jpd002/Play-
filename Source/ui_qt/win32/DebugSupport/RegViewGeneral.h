#pragma once

#include "RegViewPage.h"
#include "MIPS.h"
#include "VirtualMachine.h"
#include <string>

class CRegViewGeneral : public CRegViewPage
{
public:
	CRegViewGeneral(HWND, const RECT&, CMIPS*);
	virtual ~CRegViewGeneral() = default;

private:
	void Update() override;
	std::string GetDisplayText();

	CMIPS* m_pCtx;
};
