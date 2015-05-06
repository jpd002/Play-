#pragma once

#include "DisAsm.h"

class CDisAsmVu : public CDisAsm
{
public:
									CDisAsmVu(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CDisAsmVu();

protected:
	virtual long					OnCommand(unsigned short, unsigned short, HWND) override;

	virtual unsigned int			BuildContextMenu(HMENU) override;

	virtual std::tstring			GetInstructionDetailsText(uint32) override;
	virtual unsigned int			GetMetadataPosition() const override;

	virtual void					DrawInstructionDetails(Framework::Win32::CDeviceContext&, uint32, int) override;

private:

};
