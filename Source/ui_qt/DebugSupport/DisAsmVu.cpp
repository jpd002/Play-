#include "countof.h"
#include "DisAsmVu.h"
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "ee/VuAnalysis.h"

#define ID_DISASM_ANALYSE 40100

CDisAsmVu::CDisAsmVu(QMdiSubWindow* parent, CVirtualMachine& virtualMachine, CMIPS* ctx)
    : CDisAsm(parent, virtualMachine, ctx)
{
	m_instructionSize = 8;
}

CDisAsmVu::~CDisAsmVu()
{
}

// long CDisAsmVu::OnCommand(unsigned short id, unsigned short msg, HWND hwndFrom)
// {
// 	if(id == ID_DISASM_ANALYSE)
// 	{
// 		CVuAnalysis::Analyse(m_ctx, 0, 0x4000);
// 		Redraw();
// 		return TRUE;
// 	}
// 	return CDisAsm::OnCommand(id, msg, hwndFrom);
// }

// unsigned int CDisAsmVu::BuildContextMenu(HMENU menuHandle)
// {
// 	unsigned int position = CDisAsm::BuildContextMenu(menuHandle);
// 	InsertMenu(menuHandle, position++, MF_BYPOSITION, ID_DISASM_ANALYSE, ("Analyse Microprogram"));
// 	return position;
// }

std::string CDisAsmVu::GetInstructionDetailsText(uint32 address)
{
	assert((address & 0x07) == 0);

	uint32 lowerInstruction = GetInstruction(address + 0);
	uint32 upperInstruction = GetInstruction(address + 4);

	std::string result;

	result += lexical_cast_hex<std::string>(address, 8) + ("    ");
	result += lexical_cast_hex<std::string>(upperInstruction, 8) + (" ") + lexical_cast_hex<std::string>(lowerInstruction, 8) + ("    ");

	char disasm[256];

	m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 4, upperInstruction, disasm, countof(disasm));
	result += disasm;

	for(size_t j = strlen(disasm); j < 15; j++)
	{
		result += (" ");
	}

	m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 4, upperInstruction, disasm, countof(disasm));
	result += disasm;

	for(size_t j = strlen(disasm); j < 31; j++)
	{
		result += (" ");
	}

	m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 0, lowerInstruction, disasm, countof(disasm));
	result += disasm;

	for(size_t j = strlen(disasm); j < 16; j++)
	{
		result += (" ");
	}

	m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 0, lowerInstruction, disasm, countof(disasm));
	result += disasm;

	return result;
}

unsigned int CDisAsmVu::GetMetadataPosition() const
{
	return m_renderMetrics.fontSizeX * 111;
}

// void CDisAsmVu::DrawInstructionDetails(Framework::Win32::CDeviceContext& deviceContext, uint32 address, int y)
// {
// 	assert((address & 0x07) == 0);

// 	uint32 lowerInstruction = GetInstruction(address + 0);
// 	uint32 upperInstruction = GetInstruction(address + 4);

// 	std::string instructionCode = lexical_cast_hex<std::string>(upperInstruction, 8) + (" ") + lexical_cast_hex<std::string>(lowerInstruction, 8);
// 	deviceContext.TextOut(m_renderMetrics.fontSizeX * 15, y, instructionCode.c_str());

// 	{
// 		char disAsm[256];
// 		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 4, upperInstruction, disAsm, 256);
// 		deviceContext.TextOut(m_renderMetrics.fontSizeX * 35, y, disasm.c_str());
// 	}

// 	{
// 		char disAsm[256];
// 		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 4, upperInstruction, disAsm, 256);
// 		deviceContext.TextOut(m_renderMetrics.fontSizeX * 43, y, disasm.c_str());
// 	}

// 	{
// 		char disAsm[256];
// 		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 0, lowerInstruction, disAsm, 256);
// 		deviceContext.TextOut(m_renderMetrics.fontSizeX * 73, y, disasm.c_str());
// 	}

// 	{
// 		char disAsm[256];
// 		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 0, lowerInstruction, disAsm, 256);
// 		deviceContext.TextOut(m_renderMetrics.fontSizeX * 81, y, disasm.c_str());
// 	}
// }
