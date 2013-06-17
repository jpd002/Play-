#include "DisAsmVu.h"
#include "lexical_cast_ex.h"
#include "string_cast.h"

CDisAsmVu::CDisAsmVu(HWND hParent, const RECT& rect, CVirtualMachine& virtualMachine, CMIPS* ctx)
: CDisAsm(hParent, rect, virtualMachine, ctx)
{
	m_instructionSize = 8;
}

CDisAsmVu::~CDisAsmVu()
{

}

void CDisAsmVu::DrawInstructionDetails(Framework::Win32::CDeviceContext& deviceContext, uint32 address, int y)
{
	assert((address & 0x07) == 0);

	uint32 lowerInstruction = GetInstruction(address + 0);
	uint32 upperInstruction = GetInstruction(address + 4);

	std::tstring instructionCode = lexical_cast_hex<std::tstring>(upperInstruction, 8) + _T(" ") + lexical_cast_hex<std::tstring>(lowerInstruction, 8);
	deviceContext.TextOut(100, y, instructionCode.c_str());
		
	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 4, upperInstruction, disAsm, 256);
		deviceContext.TextOut(250, y, string_cast<std::tstring>(disAsm).c_str());
	}

	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 4, upperInstruction, disAsm, 256);
		deviceContext.TextOut(350, y, string_cast<std::tstring>(disAsm).c_str());
	}

	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 0, lowerInstruction, disAsm, 256);
		deviceContext.TextOut(650, y, string_cast<std::tstring>(disAsm).c_str());
	}

	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 0, lowerInstruction, disAsm, 256);
		deviceContext.TextOut(750, y, string_cast<std::tstring>(disAsm).c_str());
	}
}
