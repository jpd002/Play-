#include "MIPSReflection.h"
#include <string.h>

class CMIPS;

using namespace MIPSReflection;

INSTRUCTION* MIPSReflection::DereferenceInstruction(SUBTABLE* pSubTable, uint32 nOpcode)
{
	unsigned int nIndex;
	nIndex = (nOpcode >> pSubTable->nShift) & pSubTable->nMask;
	return &(pSubTable->pTable[nIndex]);
}

void MIPSReflection::CopyMnemonic(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nOpcode, char* sText, unsigned int nCount)
{
	strncpy(sText, pInstr->sMnemonic, nCount);
}

void MIPSReflection::SubTableMnemonic(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nOpcode, char* sText, unsigned int nCount)
{
	pInstr = DereferenceInstruction(pInstr->pSubTable, nOpcode);
	if(pInstr->pGetMnemonic == NULL) 
	{
		strncpy(sText, "???", nCount); 
		return;
	}
	pInstr->pGetMnemonic(pInstr, pCtx, nOpcode, sText, nCount);
}

void MIPSReflection::SubTableOperands(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode, char* sText, unsigned int nCount)
{
	pInstr = DereferenceInstruction(pInstr->pSubTable, nOpcode);
	if(pInstr->pGetOperands == NULL) 
	{
		strncpy(sText, "", nCount); 
		return;
	}
	pInstr->pGetOperands(pInstr, pCtx, nAddress, nOpcode, sText, nCount);
}

bool MIPSReflection::IsBranch(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nOpcode)
{
	return true;
}

bool MIPSReflection::SubTableIsBranch(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nOpcode)
{
	pInstr = DereferenceInstruction(pInstr->pSubTable, nOpcode);
	if(pInstr->pIsBranch == NULL)
	{
		return false;
	}
	return pInstr->pIsBranch(pInstr, pCtx, nOpcode);
}

uint32 MIPSReflection::SubTableEffAddr(INSTRUCTION* pInstr, CMIPS* pCtx, uint32 nAddress, uint32 nOpcode)
{
	pInstr = DereferenceInstruction(pInstr->pSubTable, nOpcode);
	if(pInstr->pGetEffectiveAddress == NULL)
	{
		return 0;
	}
	return pInstr->pGetEffectiveAddress(pInstr, pCtx, nAddress, nOpcode);
}
