#include <QPainter>

#include "QtDisAsmVuTableModel.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

CQtDisAsmVuTableModel::CQtDisAsmVuTableModel(QTableView* parent, CVirtualMachine& virtualMachine, CMIPS* context)
    : CQtDisAsmTableModel(parent, virtualMachine, context, DISASM_TYPE::DISASM_VU)
{
	m_headers = {"S", "Address", "R", "Instr", "UI-Mn", "UI-Op", "LI-Mn", "LI-Op", "Target/Comments"};

	m_instructionSize = 8;
}

std::string CQtDisAsmVuTableModel::GetInstructionDetails(int index, uint32 address) const
{
	assert((address & 0x07) == 0);

	uint32 lowerInstruction = GetInstruction(address + 0);
	uint32 upperInstruction = GetInstruction(address + 4);
	switch(index)
	{
	case 0:
	{
		std::string instructionCode = lexical_cast_hex<std::string>(upperInstruction, 8) + (" ") + lexical_cast_hex<std::string>(lowerInstruction, 8);
		return instructionCode;
	}
	case 1:
	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 4, upperInstruction, disAsm, 256);
		return disAsm;
	}
	case 2:
	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 4, upperInstruction, disAsm, 256);
		return disAsm;
	}
	case 3:
	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 0, lowerInstruction, disAsm, 256);
		return disAsm;
	}
	case 4:
	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 0, lowerInstruction, disAsm, 256);
		return disAsm;
	}
	}
	return "";
}
