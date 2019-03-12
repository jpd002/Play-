#pragma once

#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"
#include <string>

class CRegViewGeneral : public CRegViewPage, public boost::signals2::trackable
{
public:
	CRegViewGeneral(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual ~CRegViewGeneral() = default;

private:
	void Update() override;
	std::string GetDisplayText();

	CVirtualMachine& m_virtualMachine;
	CMIPS* m_pCtx;
};
