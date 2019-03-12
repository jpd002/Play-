#pragma once

#include <string>
#include "RegViewPage.h"
#include "../MIPS.h"
#include "../VirtualMachine.h"

class CRegViewFPU : public CRegViewPage, public boost::signals2::trackable
{
public:
	CRegViewFPU(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual ~CRegViewFPU() = default;

	void Update() override;

protected:
	long OnRightButtonUp(int, int) override;
	long OnCommand(unsigned short, unsigned short, HWND) override;

private:
	enum VIEWMODE
	{
		VIEWMODE_WORD,
		VIEWMODE_SINGLE,
		VIEWMODE_MAX,
	};

	std::string GetDisplayText();

	std::string RenderWord();
	std::string RenderSingle();
	std::string RenderFCSR();

	void OnMachineStateChange();
	void OnRunningStateChange();

	VIEWMODE m_nViewMode;
	CMIPS* m_pCtx;
};
