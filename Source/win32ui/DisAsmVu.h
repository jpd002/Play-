#pragma once

#include "DisAsm.h"

class CDisAsmVu : public CDisAsm
{
public:
									CDisAsmVu(HWND, const RECT&, CVirtualMachine&, CMIPS*);
	virtual							~CDisAsmVu();

protected:
	virtual void					DrawInstructionDetails(Framework::Win32::CDeviceContext&, uint32, int) override;

private:

};
