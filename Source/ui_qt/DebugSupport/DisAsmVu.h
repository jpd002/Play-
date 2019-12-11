#pragma once

#include "DisAsm.h"

class CDisAsmVu : public CDisAsm
{
public:
	CDisAsmVu(QMdiSubWindow*, CVirtualMachine&, CMIPS*);
	virtual ~CDisAsmVu();

protected:
	// long OnCommand(unsigned short, unsigned short, HWND) override;

	// unsigned int BuildContextMenu(HMENU) override;

	std::string GetInstructionDetailsText(uint32) override;
	unsigned int GetMetadataPosition() const override;

	// void DrawInstructionDetails(Framework::Win32::CDeviceContext&, uint32, int) override;

private:
};
