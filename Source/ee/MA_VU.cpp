#include "MA_VU.h"
#include "../MIPS.h"

CMA_VU::CMA_VU()
: CMIPSArchitecture(MIPS_REGSIZE_64)
{
	SetupReflectionTables();
}

CMA_VU::~CMA_VU()
{

}

void CMA_VU::CompileInstruction(uint32 nAddress, CMipsJitter* codeGen, CMIPS* pCtx)
{
	SetupQuickVariables(nAddress, codeGen, pCtx);

	if(nAddress & 0x04)
	{
		m_Upper.CompileInstruction(nAddress, codeGen, pCtx);
	}
	else
	{
		m_Lower.CompileInstruction(nAddress, codeGen, pCtx);
	}
}

void CMA_VU::SetRelativePipeTime(uint32 relativePipeTime)
{
	m_Lower.SetRelativePipeTime(relativePipeTime);
	m_Upper.SetRelativePipeTime(relativePipeTime);
}

void CMA_VU::SetupReflectionTables()
{
	m_Lower.SetupReflectionTables();
	m_Upper.SetupReflectionTables();
}

void CMA_VU::GetInstructionMnemonic(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	if(nAddress & 0x04)
	{
		m_Upper.GetInstructionMnemonic(pCtx, nAddress, nOpcode, sText, nCount);
	}
	else
	{
		m_Lower.GetInstructionMnemonic(pCtx, nAddress, nOpcode, sText, nCount);
	}
}

void CMA_VU::GetInstructionOperands(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	if(nAddress & 0x04)
	{
		m_Upper.GetInstructionOperands(pCtx, nAddress, nOpcode, sText, nCount);
	}
	else
	{
		m_Lower.GetInstructionOperands(pCtx, nAddress, nOpcode, sText, nCount);
	}
}

MIPS_BRANCH_TYPE CMA_VU::IsInstructionBranch(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	if(nAddress & 0x04)
	{
		return m_Upper.IsInstructionBranch(pCtx, nAddress, nOpcode);
	}
	else
	{
		return m_Lower.IsInstructionBranch(pCtx, nAddress, nOpcode);
	}
}

uint32 CMA_VU::GetInstructionEffectiveAddress(CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	if(nAddress & 0x04)
	{
		return m_Upper.GetInstructionEffectiveAddress(pCtx, nAddress, nOpcode);
	}
	else
	{
		return m_Lower.GetInstructionEffectiveAddress(pCtx, nAddress, nOpcode);
	}
}

VUShared::OPERANDSET CMA_VU::GetAffectedOperands(CMIPS* context, uint32 address, uint32 opcode)
{
	if(address & 0x04)
	{
		return m_Upper.GetAffectedOperands(context, address, opcode);
	}
	else
	{
		return m_Lower.GetAffectedOperands(context, address, opcode);
	}
}
