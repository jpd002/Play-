#pragma once

#include "DisAsm.h"

class CDisAsmVu : public CDisAsm
{
public:
	CDisAsmVu(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual ~CDisAsmVu();

protected:
	long OnCommand(unsigned short, unsigned short, HWND) override;

	unsigned int BuildContextMenu(HMENU) override;

	std::tstring GetInstructionDetailsText(uint32) override;
	unsigned int GetMetadataPosition() const override;

	void DrawInstructionDetails(Framework::Win32::CDeviceContext&, uint32, int) override;

private:
};
