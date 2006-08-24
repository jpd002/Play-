#include <assert.h>
#include "CodeGen.h"
#include "CodeGen_VUI128.h"
#include "CodeGen_VUF128.h"
#include "CodeGen_FPU.h"
#include "PtrMacro.h"

bool					CCodeGen::m_nBlockStarted = false;
CCacheBlock*			CCodeGen::m_pBlock = NULL;
CArrayStack<uint32>		CCodeGen::m_Shadow;
#ifdef AMD64
CArrayStack<uint32, 2>	CCodeGen::m_PullReg64Stack;
#endif
CArrayStack<uint32>		CCodeGen::m_IfStack;

bool				CCodeGen::m_nRegisterAllocated[MAX_REGISTER];

#ifdef AMD64

unsigned int		CCodeGen::m_nRegisterLookup[MAX_REGISTER] =
{
	0,
	1,
	2,
	3,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
};

#else

unsigned int		CCodeGen::m_nRegisterLookup[MAX_REGISTER] =
{
	0,
	1,
	2,
	3,
	6,
	7,
};

#endif

void CCodeGen::Begin(CCacheBlock* pBlock)
{
	unsigned int i;

	assert(m_nBlockStarted == false);
	m_nBlockStarted = true;

	CodeGen::CFPU::Begin(pBlock);
	CodeGen::CVUI128::Begin(pBlock);
	CodeGen::CVUF128::Begin(pBlock);

	m_Shadow.Reset();
	m_pBlock = pBlock;

	for(i = 0; i < MAX_REGISTER; i++)
	{
		m_nRegisterAllocated[i] = false;		
	}
}

void CCodeGen::End()
{
	assert(m_nBlockStarted == true);
	m_nBlockStarted = false;

	CodeGen::CFPU::End();
	CodeGen::CVUI128::End();
	CodeGen::CVUF128::End();
}

void CCodeGen::BeginIf(bool nCondition)
{
	if(m_Shadow.GetAt(0) == REGISTER)
	{
		uint32 nRegister, nPosition;

		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		if(!RegisterHasNextUse(nRegister))
		{
			FreeRegister(nRegister);
		}

		//test reg, reg
		m_pBlock->StreamWrite(2, 0x85, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

		//jcc label
		m_pBlock->StreamWrite(1, nCondition ? 0x74 : 0x75);
		nPosition = m_pBlock->StreamGetSize();
		m_pBlock->StreamWrite(1, 0x00);

		m_IfStack.Push(nPosition);
		m_IfStack.Push(IFBLOCK);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::EndIf()
{
	uint32 nPosition, nSize;

	assert(m_IfStack.GetAt(0) == IFBLOCK);

	m_IfStack.Pull();
	nPosition = m_IfStack.Pull();

	nSize = m_pBlock->StreamGetSize() - nPosition - 1;
	assert(nSize <= 0xFF);

	m_pBlock->StreamWriteAt(nPosition, (uint8)nSize);
}

void CCodeGen::BeginIfElse(bool nCondition)
{
	if(m_Shadow.GetAt(0) == REGISTER)
	{
		uint32 nRegister, nPosition;

		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		if(!RegisterHasNextUse(nRegister))
		{
			FreeRegister(nRegister);
		}

		//test reg, reg
		m_pBlock->StreamWrite(2, 0x85, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

		//jcc label
		m_pBlock->StreamWrite(1, nCondition ? 0x74 : 0x75);
		nPosition = m_pBlock->StreamGetSize();
		m_pBlock->StreamWrite(1, 0x00);

		m_IfStack.Push(nPosition);
		m_IfStack.Push(IFELSEBLOCK);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::BeginIfElseAlt()
{
	uint32 nPosition1, nPosition2, nSize;

	assert(m_IfStack.GetAt(0) == IFELSEBLOCK);

	m_IfStack.Pull();
	nPosition1 = m_IfStack.Pull();

	//jmp label
	m_pBlock->StreamWrite(1, 0xEB);
	nPosition2 = m_pBlock->StreamGetSize();
	m_pBlock->StreamWrite(1, 0x00);

	nSize = m_pBlock->StreamGetSize() - nPosition1 - 1;
	assert(nSize <= 0xFF);

	m_pBlock->StreamWriteAt(nPosition1, (uint8)nSize);

	m_IfStack.Push(nPosition2);
	m_IfStack.Push(IFBLOCK);
}

unsigned int CCodeGen::GetMinimumConstantSize(uint32 nConstant)
{
	if(((int32)nConstant >= -128) && ((int32)nConstant <= 127))
	{
		return 1;
	}
	return 4;
}

bool CCodeGen::RegisterHasNextUse(unsigned int nRegister)
{
	unsigned int i, nCount;

	nCount = m_Shadow.GetCount();

	for(i = 0; i < nCount; i += 2)
	{
		if(m_Shadow.GetAt(i) == REGISTER)
		{
			if(m_Shadow.GetAt(i + 1) == nRegister) return true;
		}
	}

	return false;
}

unsigned int CCodeGen::AllocateRegister(REGISTER_TYPE nPreference)
{
	if(nPreference == REGISTER_NORMAL)
	{
		for(unsigned int i = 0; i < MAX_REGISTER; i++)
		{
			if(!m_nRegisterAllocated[i])
			{
				m_nRegisterAllocated[i] = true;
				return i;
			}
		}
	}
	if(nPreference == REGISTER_SHIFTAMOUNT)
	{
		if(!m_nRegisterAllocated[1])
		{
			m_nRegisterAllocated[1] = true;
			return 1;
		}
	}
	if(nPreference == REGISTER_HASLOW)
	{
		//Only four first registers (eax, ecx, edx, ebx) has low counter parts

		for(unsigned int i = 0; i < 4; i++)
		{
			if(!m_nRegisterAllocated[i])
			{
				m_nRegisterAllocated[i] = true;
				return i;
			}
		}
	}
	if(nPreference == REGISTER_SAVED)
	{
		for(unsigned int i = 0; i < MAX_REGISTER; i++)
		{
			if(i == 0) continue;
			if(i == 1) continue;
			if(i == 2) continue;
#ifdef AMD64
			if(i == 8) continue;
			if(i == 9) continue;
			if(i == 10) continue;
			if(i == 11) continue;
#endif
			if(!m_nRegisterAllocated[i])
			{
				m_nRegisterAllocated[i] = true;
				return i;
			}
		}

	}
	assert(0);
	return 0;
}

void CCodeGen::FreeRegister(unsigned int nRegister)
{
	m_nRegisterAllocated[nRegister] = false;
}

void CCodeGen::LoadVariableInRegister(unsigned int nRegister, uint32 nVariable)
{
	//mov reg, dword ptr[Variable]
	m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
	m_pBlock->StreamWriteWord(nVariable);
}

void CCodeGen::LoadRelativeInRegister(unsigned int nRegister, uint32 nOffset)
{

#ifdef AMD64

	if(m_nRegisterLookup[nRegister] > 7)
	{
		//REX byte
		m_pBlock->StreamWrite(1, 0x44);
	}

#endif

	//mov reg, [base + rel]
	m_pBlock->StreamWrite(1, 0x8B);
	WriteRelativeRmRegister(nRegister, nOffset);
}

void CCodeGen::LoadConstantInRegister(unsigned int nRegister, uint32 nConstant)
{
	unsigned int nRegIndex;

	nRegIndex = m_nRegisterLookup[nRegister];

#ifdef AMD64

	if(nRegIndex > 7)
	{
		//REX byte
		m_pBlock->StreamWrite(1, 0x41);

		nRegIndex &= 0x07;
	}

#endif

	//mov reg, nConstant
	m_pBlock->StreamWrite(1, 0xB8 | nRegIndex);
	m_pBlock->StreamWriteWord(nConstant);
}

void CCodeGen::CopyRegister(unsigned int nDst, unsigned int nSrc)
{
	unsigned int nRegIndex1, nRegIndex2;

	nRegIndex1 = m_nRegisterLookup[nSrc];
	nRegIndex2 = m_nRegisterLookup[nDst];

#ifdef AMD64

	bool nNeedRex;
	uint8 nRex;

	nNeedRex = false;
	nRex = 0x40;

	nNeedRex = (nRegIndex1 > 7) || (nRegIndex2 > 7);

	if(nRegIndex1 > 7)
	{
		nRegIndex1 &= 0x07;
		nRex |= 0x04;
	}

	if(nRegIndex2 > 7)
	{
		nRegIndex2 &= 0x07;
		nRex |= 0x01;
	}

	if(nNeedRex)
	{
		m_pBlock->StreamWrite(1, nRex);
	}

#endif

	m_pBlock->StreamWrite(2, 0x89, 0xC0 | (nRegIndex1 << 3) | (nRegIndex2));
}

#ifdef AMD64

void CCodeGen::LoadRelativeInRegister64(unsigned int nRegister, uint32 nRelative)
{
	unsigned int nRegIndex;
	uint8 nRex;

	nRegIndex	= m_nRegisterLookup[nRegister];
	nRex		= 0x48;
	
	if(nRegIndex > 7)
	{
		nRex |= 0x01;
		nRegIndex &= 0x07;
	}

	//mov reg, nRelative
	m_pBlock->StreamWrite(2, nRex, 0x8B);
	WriteRelativeRmRegister(nRegister, nRelative);
}

void CCodeGen::LoadConstantInRegister64(unsigned int nRegister, uint64 nConstant)
{
	unsigned int nRegIndex;
	uint8 nRex;

	nRegIndex	= m_nRegisterLookup[nRegister];
	nRex		= 0x48;
	
	if(nRegIndex > 7)
	{
		nRex |= 0x01;
		nRegIndex &= 0x07;
	}

	//mov reg, nConstant
	m_pBlock->StreamWrite(2, nRex, 0xB8 | nRegIndex);
	m_pBlock->StreamWriteWord((uint32)((nConstant >>  0) & 0xFFFFFFFF));
	m_pBlock->StreamWriteWord((uint32)((nConstant >> 32) & 0xFFFFFFFF));
}

#endif

void CCodeGen::LoadConditionInRegister(unsigned int nRegister, CONDITION nCondition)
{
	//setcc reg[l]
	m_pBlock->StreamWrite(1, 0x0F);
	switch(nCondition)
	{
	case CONDITION_BL:
		m_pBlock->StreamWrite(1, 0x92);
		break;
	case CONDITION_EQ:
		m_pBlock->StreamWrite(1, 0x94);
		break;
	case CONDITION_LE:
		m_pBlock->StreamWrite(1, 0x9E);
		break;
	case CONDITION_GT:
		m_pBlock->StreamWrite(1, 0x9F);
		break;
	default:
		assert(0);
		break;
	}

	m_pBlock->StreamWrite(1, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

	//movzx reg, reg[l]
	m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
}

void CCodeGen::ReduceToRegister()
{
	if(m_Shadow.GetAt(0) == RELATIVE)
	{
		uint32 nRelative;
		unsigned int nRegister;

		m_Shadow.Pull();
		nRelative = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, nRelative);

		PushReg(nRegister);
	}
	else if(m_Shadow.GetAt(0) == VARIABLE)
	{
		uint32 nVariable, nRegister;

		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable);

		PushReg(nRegister);
	}
	else if(m_Shadow.GetAt(0) == REGISTER)
	{
		//That's fine
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::WriteRelativeRm(unsigned int nIndex, uint32 nOffset)
{
	if(nOffset <= 0x7F)
	{
		m_pBlock->StreamWrite(2, (0x01 << 6) | (nIndex << 3) | 0x05, (uint8)nOffset);
	}
	else
	{
		m_pBlock->StreamWrite(1, (0x02 << 6) | (nIndex << 3) | 0x05);
		m_pBlock->StreamWriteWord(nOffset);
	}
}

void CCodeGen::WriteRelativeRmRegister(unsigned int nRegister, uint32 nOffset)
{
	WriteRelativeRm(m_nRegisterLookup[nRegister] & 0x07, nOffset);
}

void CCodeGen::WriteRelativeRmFunction(unsigned int nFunction, uint32 nOffset)
{
	WriteRelativeRm(nFunction, nOffset);
}

uint8 CCodeGen::MakeRegFunRm(unsigned int nDst, unsigned int nFunction)
{
	assert(m_nRegisterLookup[nDst] < 8);

	return 0xC0 | (nFunction << 3) | (m_nRegisterLookup[nDst]);
}

uint8 CCodeGen::MakeRegRegRm(unsigned int nDst, unsigned int nSrc)
{
	assert(m_nRegisterLookup[nDst] < 8);
	assert(m_nRegisterLookup[nSrc] < 8);

	return 0xC0 | (m_nRegisterLookup[nSrc] << 3) | (m_nRegisterLookup[nDst]);
}

void CCodeGen::PushVar(uint32* pValue)
{
	m_Shadow.Push(*(uint32*)&pValue);
	m_Shadow.Push(VARIABLE);
}

void CCodeGen::PushCst(uint32 nValue)
{
	m_Shadow.Push(nValue);
	m_Shadow.Push(CONSTANT);
}

void CCodeGen::PushRef(void* pAddr)
{
	m_Shadow.Push(*(uint32*)&pAddr);
	m_Shadow.Push(REFERENCE);
}

void CCodeGen::PushRel(size_t nOffset)
{
	m_Shadow.Push((uint32)nOffset);
	m_Shadow.Push(RELATIVE);
}

void CCodeGen::PushIdx(unsigned int nIndex)
{
	uint32 nParam1, nParam2;

	nIndex *= 2;

	nParam1 = m_Shadow.GetAt(nIndex + 0);
	nParam2 = m_Shadow.GetAt(nIndex + 1);

	m_Shadow.Push(nParam2);
	m_Shadow.Push(nParam1);
}

void CCodeGen::PushTop()
{
	PushIdx(0);
}

void CCodeGen::PushReg(unsigned int nRegister)
{
	m_Shadow.Push(nRegister);
	m_Shadow.Push(REGISTER);
}

#ifdef AMD64

void CCodeGen::PushReg64(unsigned int nRegister)
{
	m_Shadow.Push(nRegister);
	m_Shadow.Push(REGISTER64);
}

#endif

void CCodeGen::ReplaceRegisterInStack(unsigned int nDst, unsigned int nSrc)
{
	for(unsigned int i = 0; i < m_Shadow.GetCount(); i += 2)
	{
		if(m_Shadow.GetAt(i + 0) == REGISTER)
		{
			unsigned int nRegister;
			nRegister = m_Shadow.GetAt(i + 1);
			if(nRegister == nSrc)
			{
				m_Shadow.SetAt(i + 1, nDst);
			}
		}
	}
}

void CCodeGen::PullVar(uint32* pValue)
{
	if(m_Shadow.GetAt(0) == REGISTER)
	{
		uint32 nRegister;

		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		//mov dword ptr[pValue], reg
		m_pBlock->StreamWrite(2, 0x89, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(*(uint32*)&pValue);

		FreeRegister(nRegister);
	}
	else if(m_Shadow.GetAt(0) == CONSTANT)
	{
		uint32 nConstant, nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();

		nRegister = AllocateRegister();

		if(nConstant == 0)
		{
			//xor reg, reg
			m_pBlock->StreamWrite(2, 0x33, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		}
		else
		{
			//mov reg, $Constant
			m_pBlock->StreamWrite(1, 0xB8 | (m_nRegisterLookup[nRegister]));
			m_pBlock->StreamWriteWord(nConstant);
		}

		//mov dword ptr[pValue], reg
		m_pBlock->StreamWrite(2, 0x89, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(*(uint32*)&pValue);

		FreeRegister(nRegister);
	}
	else if(m_Shadow.GetAt(0) == VARIABLE)
	{
		uint32 nVariable, nRegister;

		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();

		//mov reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		PullVar(pValue);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::PullRel(size_t nOffset)
{
	if(m_Shadow.GetAt(0) == REGISTER)
	{
		uint32 nRegister;

		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		//mov dword ptr[pValue], reg
		m_pBlock->StreamWrite(1, 0x89);
		WriteRelativeRmRegister(nRegister, (uint32)nOffset);

		FreeRegister(nRegister);
	}
	else if(m_Shadow.GetAt(0) == CONSTANT)
	{
		uint32 nConstant, nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();

		nRegister = AllocateRegister();

		if(nConstant == 0)
		{
			//xor reg, reg
			m_pBlock->StreamWrite(2, 0x33, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		}
		else
		{
			//mov reg, $Constant
			m_pBlock->StreamWrite(1, 0xB8 | (m_nRegisterLookup[nRegister]));
			m_pBlock->StreamWriteWord(nConstant);
		}

		//mov dword ptr[pValue], reg
		m_pBlock->StreamWrite(1, 0x89);
		WriteRelativeRmRegister(nRegister, (uint32)nOffset);

		FreeRegister(nRegister);		
	}
	else if(m_Shadow.GetAt(0) == VARIABLE)
	{
		ReduceToRegister();

		PullRel(nOffset);
	}
	else if(m_Shadow.GetAt(0) == RELATIVE)
	{
		uint32 nRelative, nRegister;

		m_Shadow.Pull();
		nRelative = m_Shadow.Pull();

		nRegister = AllocateRegister();

		LoadRelativeInRegister(nRegister, nRelative);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		PullRel(nOffset);
	}
#ifdef AMD64
	else if(m_Shadow.GetAt(0) == REGISTER64)
	{
		if(m_PullReg64Stack.GetCount() == 0)
		{
			//Save the provided offset for the next time
			m_PullReg64Stack.Push(static_cast<uint32>(nOffset));
		}
		else
		{
			uint32 nRelative1, nRelative2;
			uint32 nRegister;

			nRelative2 = m_PullReg64Stack.Pull();
			nRelative1 = static_cast<uint32>(nOffset);

			m_Shadow.Pull();
			nRegister = m_Shadow.Pull();

			assert((nRelative2 - nRelative1) == 4);
			assert(m_nRegisterLookup[nRegister] < 8);

			//mov qword ptr[pValue], reg
			m_pBlock->StreamWrite(2, 0x48, 0x89);
			WriteRelativeRmRegister(nRegister, nRelative1);

			FreeRegister(nRegister);
		}
	}
#endif
	else
	{
		assert(0);
	}
}

void CCodeGen::PullTop()
{
	m_Shadow.Pull();
	m_Shadow.Pull();
}

void CCodeGen::Add()
{
	if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == VARIABLE))
	{
		uint32 nVariable, nConstant, nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();
		
		//mov reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable);

		if(GetMinimumConstantSize(nConstant) == 1)
		{
			//add reg, Immediate
			m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]), (uint8)nConstant);
		}
		else
		{
			//add reg, Immediate
			m_pBlock->StreamWrite(2, 0x81, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));
			m_pBlock->StreamWriteWord(nConstant);
		}

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == RELATIVE))
	{
		uint32 nRelative, nConstant, nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();

		//No point in adding zero
		if(nConstant == 0) return;

		m_Shadow.Pull();
		nRelative = m_Shadow.Pull();

		nRegister = AllocateRegister();

		LoadRelativeInRegister(nRegister, nRelative);

		if(GetMinimumConstantSize(nConstant) == 1)
		{
			//add reg, Immediate
			m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]), (uint8)nConstant);
		}
		else
		{
			//add reg, Immediate
			m_pBlock->StreamWrite(2, 0x81, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));
			m_pBlock->StreamWriteWord(nConstant);
		}

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);		
	}
	else if((m_Shadow.GetAt(0) == VARIABLE) && (m_Shadow.GetAt(2) == VARIABLE))
	{
		uint32 nVariable1, nVariable2, nRegister;

		m_Shadow.Pull();
		nVariable2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable1 = m_Shadow.Pull();

		nRegister	= AllocateRegister();

		//mov reg, dword ptr[Variable1]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable1);

		//add reg, dword ptr[Variable2]
		m_pBlock->StreamWrite(2, 0x03, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == REGISTER) && (m_Shadow.GetAt(2) == REGISTER))
	{
		uint32 nRegister1, nRegister2;
	
		m_Shadow.Pull();
		nRegister2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRegister1 = m_Shadow.Pull();

		//Can free this register
		if(!RegisterHasNextUse(nRegister2))
		{
			FreeRegister(nRegister2);
		}

		//add reg1, reg2
		m_pBlock->StreamWrite(2, 0x03, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister2]));

		m_Shadow.Push(nRegister1);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == REGISTER))
	{
		uint32 nRegister, nConstant;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();

		//No point in adding zero
		if(nConstant == 0) return;

		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		if(GetMinimumConstantSize(nConstant) == 1)
		{
			//add reg, Immediate
			m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]), (uint8)nConstant);
		}
		else
		{
			//add reg, Immediate
			m_pBlock->StreamWrite(2, 0x81, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));
			m_pBlock->StreamWriteWord(nConstant);
		}

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == CONSTANT))
	{
		uint32 nConstant1, nConstant2;
	
		m_Shadow.Pull();
		nConstant2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nConstant1 = m_Shadow.Pull();

		PushCst(nConstant2 + nConstant1);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Add64()
{
	if(\
		(m_Shadow.GetAt(0) == VARIABLE) && \
		(m_Shadow.GetAt(2) == VARIABLE) && \
		(m_Shadow.GetAt(4) == VARIABLE) && \
		(m_Shadow.GetAt(6) == VARIABLE))
	{
		uint32 nVariable1, nVariable2, nVariable3, nVariable4;
		uint32 nRegister1, nRegister2;

		m_Shadow.Pull();
		nVariable4 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable3 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable1 = m_Shadow.Pull();

		nRegister1 = AllocateRegister();
		nRegister2 = AllocateRegister();

		//mov reg1, dword ptr[Variable1]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister1] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable1);

		//mov reg2, dword ptr[Variable2]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister2] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2);

		//add reg1, dword ptr[Variable3]
		m_pBlock->StreamWrite(2, 0x03, 0x00 | (m_nRegisterLookup[nRegister1] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable3);

		//adc reg2, dword ptr[Variable4]
		m_pBlock->StreamWrite(2, 0x13, 0x00 | (m_nRegisterLookup[nRegister2] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable4);

		m_Shadow.Push(nRegister1);
		m_Shadow.Push(REGISTER);

		m_Shadow.Push(nRegister2);
		m_Shadow.Push(REGISTER);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::And()
{
	if((m_Shadow.GetAt(0) == REGISTER) && (m_Shadow.GetAt(2) == REGISTER))
	{
		uint32 nRegister1, nRegister2;

		m_Shadow.Pull();
		nRegister2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRegister1 = m_Shadow.Pull();

		//This register will change... Make sure there's no next uses
		if(RegisterHasNextUse(nRegister1))
		{
			//Must save or something...
			assert(0);
		}

		//Can free this register
		if(!RegisterHasNextUse(nRegister2))
		{
			FreeRegister(nRegister2);
		}

		//and reg1, reg2
		m_pBlock->StreamWrite(2, 0x23, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister2]));

		m_Shadow.Push(nRegister1);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == REGISTER))
	{
		uint32 nConstant;
		unsigned int nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();
		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		if(GetMinimumConstantSize(nConstant) == 1)
		{
			//and reg, Immediate
			m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x04 << 3) | (m_nRegisterLookup[nRegister]), (uint8)nConstant);
		}
		else
		{
			//and reg, Immediate
			m_pBlock->StreamWrite(2, 0x81, 0xC0 | (0x04 << 3) | (m_nRegisterLookup[nRegister]));
			m_pBlock->StreamWriteWord(nConstant);
		}

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == VARIABLE))
	{
		uint32 nVariable, nConstant;
		unsigned int nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		m_Shadow.Push(nConstant);
		m_Shadow.Push(CONSTANT);

		And();
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == RELATIVE))
	{
		uint32 nRelative, nConstant;
		unsigned int nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, nRelative);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		m_Shadow.Push(nConstant);
		m_Shadow.Push(CONSTANT);

		And();
	}
	else if((m_Shadow.GetAt(0) == VARIABLE) && (m_Shadow.GetAt(2) == VARIABLE))
	{
		uint32 nVariable1, nVariable2;
		uint32 nRegister;

		m_Shadow.Pull();
		nVariable2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable1 = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable1);

		//and reg, [nVariable2]
		m_pBlock->StreamWrite(2, 0x23, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == CONSTANT))
	{
		uint32 nConstant1, nConstant2;
	
		m_Shadow.Pull();
		nConstant2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nConstant1 = m_Shadow.Pull();

		PushCst(nConstant2 & nConstant1);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Call(void* pFunc, unsigned int nParamCount, bool nKeepRet)
{
	uint32 nCallRegister;

#ifdef AMD64

	unsigned int nParamReg[4] =
	{
		1,
		2,
		6,
		7,
	};

	assert(nParamCount <= 4);
	assert(m_nRegisterAllocated[nParamReg[0]] == false);
	assert(m_nRegisterAllocated[nParamReg[1]] == false);
	assert(m_nRegisterAllocated[nParamReg[2]] == false);
	assert(m_nRegisterAllocated[nParamReg[3]] == false);

#endif

	for(unsigned int i = 0; i < nParamCount; i++)
	{
		if(m_Shadow.GetAt(0) == VARIABLE)
		{
			uint32 nVariable;

			m_Shadow.Pull();
			nVariable = m_Shadow.Pull();

			//push [nVariable]
			m_pBlock->StreamWrite(2, 0xFF, 0x35);
			m_pBlock->StreamWriteWord(nVariable);
		}
		else if(m_Shadow.GetAt(0) == RELATIVE)
		{
			uint32 nRelative;

			m_Shadow.Pull();
			nRelative = m_Shadow.Pull();

#ifdef AMD64

			LoadRelativeInRegister(nParamReg[nParamCount - 1 - i], nRelative);

#else

			//push [nBase + nRel]
			m_pBlock->StreamWrite(1, 0xFF);
			WriteRelativeRmFunction(6, nRelative);

#endif

		}

		else if(m_Shadow.GetAt(0) == REGISTER)
		{
			uint32 nRegister;

			m_Shadow.Pull();
			nRegister = m_Shadow.Pull();

			if(!RegisterHasNextUse(nRegister))
			{
				FreeRegister(nRegister);
			}

#ifdef AMD64

			//mov param_reg, reg
			CopyRegister(nParamReg[nParamCount - 1 - i], nRegister);

#else

			//push reg
			m_pBlock->StreamWrite(1, 0x50 | m_nRegisterLookup[nRegister]);

#endif

		}
		else if((m_Shadow.GetAt(0) == REFERENCE) || (m_Shadow.GetAt(0) == CONSTANT))
		{
			uint32 nNumber;

			m_Shadow.Pull();
			nNumber = m_Shadow.Pull();

#ifdef AMD64

			//mov reg, nNumber
			LoadConstantInRegister(nParamReg[nParamCount - 1 - i], nNumber);

#else

			//push nNumber
			m_pBlock->StreamWrite(1, 0x68);
			m_pBlock->StreamWriteWord(nNumber);

#endif

		}
		else
		{
			assert(0);
		}
	}

	if(m_nRegisterAllocated[0])
	{
		//This register is going to be scrapped, gotta save it...
		unsigned int nSaveReg;

		nSaveReg = AllocateRegister(REGISTER_SAVED);
		CopyRegister(nSaveReg, 0);
		ReplaceRegisterInStack(nSaveReg, 0);

		FreeRegister(0);
	}

	nCallRegister = AllocateRegister();

#ifdef AMD64

	unsigned int nRegIndex;

	if(nParamCount != 0)
	{
		//sub rsp, (nParams * 0x08)
		m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x05 << 3) | (0x04), nParamCount * 0x08);
	}

	//mov reg, nFuncAddress
	LoadConstantInRegister64(nCallRegister, (uint64)((uint8*)pFunc - (uint8*)0));

	//call reg
	nRegIndex = m_nRegisterLookup[nCallRegister];
	if(nRegIndex > 7)
	{
		//REX byte
		m_pBlock->StreamWrite(1, 0x41);
		nRegIndex &= 7;
	}

	m_pBlock->StreamWrite(2, 0xFF, 0xD0 | (nRegIndex));

	if(nParamCount != 0)
	{
		//add esp, 0x08
		m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x00 << 3) | (0x04), nParamCount * 0x08);
	}

#else

	//mov reg, pFunc
	m_pBlock->StreamWrite(1, 0xB8 | (m_nRegisterLookup[nCallRegister]));
	m_pBlock->StreamWriteWord((uint32)((uint8*)pFunc - (uint8*)0));

	//call reg
	m_pBlock->StreamWrite(2, 0xFF, 0xD0 | (m_nRegisterLookup[nCallRegister]));

	if(nParamCount != 0)
	{
		//add esp, nParams * 4;
		m_pBlock->StreamWrite(3, 0x83, 0xC4, nParamCount * 4);
	}

#endif

	FreeRegister(nCallRegister);

	if(nKeepRet)
	{
		assert(m_nRegisterAllocated[0] == false);
		m_nRegisterAllocated[0] = true;

		m_Shadow.Push(0);
		m_Shadow.Push(REGISTER);
	}
}

void CCodeGen::Cmp(CONDITION nCondition)
{
	if((m_Shadow.GetAt(0) == VARIABLE) && (m_Shadow.GetAt(2) == VARIABLE))
	{
		uint32 nVariable1, nVariable2, nRegister;

		m_Shadow.Pull();
		nVariable2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable1 = m_Shadow.Pull();

		nRegister = AllocateRegister();

		//mov reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable1);

		//cmp reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x3B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2);

		//setcc reg[l]
		m_pBlock->StreamWrite(1, 0x0F);
		switch(nCondition)
		{
		case CONDITION_BL:
			m_pBlock->StreamWrite(1, 0x92);
			break;
		case CONDITION_EQ:
			m_pBlock->StreamWrite(1, 0x94);
			break;
		default:
			assert(0);
			break;
		}

		m_pBlock->StreamWrite(1, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

		//movzx reg, reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(2) == VARIABLE) && (m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(1) == 0) && (nCondition == CONDITION_EQ))
	{
		uint32 nVariable, nRegister;

		m_Shadow.Pull();
		m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();

		//mov reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable);

		//test reg, reg
		m_pBlock->StreamWrite(2, 0x85, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		
		//sete reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

		//movzx reg, reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		
		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if(IsTopRegZeroPairCom() && (nCondition == CONDITION_EQ))
	{
		unsigned int nRegister;

		GetRegCstPairCom(&nRegister, NULL);

		//test reg, reg
		m_pBlock->StreamWrite(2, 0x85, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		
		//sete reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

		//movzx reg, reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		
		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(2) == REGISTER) && (m_Shadow.GetAt(0) == CONSTANT))
	{
		uint32 nConstant;
		unsigned int nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();
		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		if(GetMinimumConstantSize(nConstant) == 1)
		{
			//cmp reg, Immediate
			m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x07 << 3) | (m_nRegisterLookup[nRegister]), (uint8)nConstant);
		}
		else
		{
			//cmp reg, Immediate
			m_pBlock->StreamWrite(2, 0x81, 0xC0 | (0x07 << 3) | (m_nRegisterLookup[nRegister]));
			m_pBlock->StreamWriteWord(nConstant);
		}

		LoadConditionInRegister(nRegister, nCondition);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Cmp64(CONDITION nCondition)
{
	switch(nCondition)
	{
	case CONDITION_BL:
	case CONDITION_LT:
		Cmp64Lt(nCondition == CONDITION_LT);
		break;
	case CONDITION_EQ:
		Cmp64Eq();
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen::Lzc()
{
	if(m_Shadow.GetAt(0) == VARIABLE)
	{
		uint32 nVariable;
		unsigned int nRegister;

		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable);

		//cmp reg, -1
		m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x07 << 3) | (m_nRegisterLookup[nRegister]), 0xFF);

		//je __set32 (+0x12)
		m_pBlock->StreamWrite(2, 0x74, 0x12);

		//test reg, reg
		m_pBlock->StreamWrite(2, 0x85, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

		//je __set32 (+0x0E)
		m_pBlock->StreamWrite(2, 0x74, 0x0E);

		//jns __countzero (+0x02)
		m_pBlock->StreamWrite(2, 0x79, 0x02);

		//__countone
		//not reg
		m_pBlock->StreamWrite(2, 0xF7, 0xC0 | (0x02 << 3) | (m_nRegisterLookup[nRegister]));

		//__countzero
		//bsr reg, reg
		m_pBlock->StreamWrite(3, 0x0F, 0xBD, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

		//neg reg
		m_pBlock->StreamWrite(2, 0xF7, 0xC0 | (0x03 << 3) | (m_nRegisterLookup[nRegister]));

		//add reg, 0x1E
		m_pBlock->StreamWrite(3, 0x83, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]), 0x1E);

		//jmp __done (+0x05)
		m_pBlock->StreamWrite(2, 0xEB, 0x05);

		//__set32
		//mov reg, 0x1F
		m_pBlock->StreamWrite(1, 0xB8 | (m_nRegisterLookup[nRegister]));
		m_pBlock->StreamWriteWord(0x0000001F);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
/*
		__asm
		{
			mov ebx, 0;
			cmp ebx, -1;
			je _set32;
			test ebx, ebx;
			jz _set32;
			jns _countzero;
_countone:
			not ebx;
_countzero:
			bsr ebx, ebx;
			neg ebx;
			add ebx, 30;
			jmp _done;
_set32:
			mov ebx, 31
_done:

		}
*/
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Or()
{
	if((m_Shadow.GetAt(0) == REGISTER) && (m_Shadow.GetAt(2) == REGISTER))
	{
		uint32 nRegister1, nRegister2;

		m_Shadow.Pull();
		nRegister2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRegister1 = m_Shadow.Pull();

		//This register will change... Make sure there's no next uses
		if(RegisterHasNextUse(nRegister1))
		{
			//Must save or something...
			assert(0);
		}

		//Can free this register
		if(!RegisterHasNextUse(nRegister2))
		{
			FreeRegister(nRegister2);
		}

		//or reg1, reg2
		m_pBlock->StreamWrite(2, 0x0B, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister2]));

		m_Shadow.Push(nRegister1);
		m_Shadow.Push(REGISTER);
	}
	else if(IsTopRegCstPairCom())
	{
		uint32 nConstant;
		unsigned int nRegister;

		GetRegCstPairCom(&nRegister, &nConstant);

		//or reg, const
		m_pBlock->StreamWrite(2, 0x81, 0xC0 | (0x01 << 3) | (m_nRegisterLookup[nRegister]));
		m_pBlock->StreamWriteWord(nConstant);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == VARIABLE))
	{
		unsigned int nRegister;
		uint32 nConstant, nVariable;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		m_Shadow.Push(nConstant);
		m_Shadow.Push(CONSTANT);

		Or();
	}
	else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == RELATIVE))
	{
		unsigned int nRegister;
		uint32 nConstant, nRelative;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, nRelative);

		PushReg(nRegister);
		PushCst(nConstant);

		Or();
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::SeX16()
{
	ReduceToRegister();

	uint32 nRegister;

	m_Shadow.Pull();
	nRegister = m_Shadow.Pull();

	assert(nRegister == 0);

	//cwde
	m_pBlock->StreamWrite(1, 0x98);

	m_Shadow.Push(0);
	m_Shadow.Push(REGISTER);
}

void CCodeGen::SeX()
{
	ReduceToRegister();

	uint32 nRegister;

	m_Shadow.Pull();
	nRegister = m_Shadow.Pull();

	if(nRegister != 0 && m_nRegisterAllocated[2])
	{
		assert(0);
	}

	m_nRegisterAllocated[2] = true;

	//cdq
	m_pBlock->StreamWrite(1, 0x99);

	m_Shadow.Push(0);
	m_Shadow.Push(REGISTER);

	m_Shadow.Push(2);
	m_Shadow.Push(REGISTER);
}

void CCodeGen::Shl(uint8 nAmount)
{
	if(m_Shadow.GetAt(0) == REGISTER)
	{
		uint32 nRegister;

		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		//shr nRegister, nAmount
		m_pBlock->StreamWrite(3, 0xC1, 0xE0 | (m_nRegisterLookup[nRegister]), nAmount);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if(m_Shadow.GetAt(0) == RELATIVE)
	{
		uint32 nRelative;
		unsigned int nRegister;

		m_Shadow.Pull();
		nRelative = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, nRelative);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		Shl(nAmount);
	}
	else if(m_Shadow.GetAt(0) == VARIABLE)
	{
		uint32 nVariable;
		unsigned int nRegister;

		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		Shl(nAmount);
	}
	else if(m_Shadow.GetAt(0) == CONSTANT)
	{
		uint32 nConstant;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();

		nConstant <<= nAmount;

		PushCst(nConstant);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Shl64()
{
	if(\
		(m_Shadow.GetAt(0) == RELATIVE) && \
		(m_Shadow.GetAt(2) == RELATIVE) && \
		(m_Shadow.GetAt(4) == RELATIVE))
	{
		uint32 nAmount, nRelative1, nRelative2;
		unsigned int nAmountReg;

		m_Shadow.Pull();
		nAmount = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		nAmountReg = AllocateRegister(REGISTER_SHIFTAMOUNT);
		LoadRelativeInRegister(nAmountReg, nAmount);

		//and cl, 0x3F
		m_pBlock->StreamWrite(3, 0x80, 0xC0 | (0x4 << 3) | 1, 0x3F);

#ifdef AMD64
		if((nRelative2 - nRelative1) == 4)
		{
			unsigned int nRegister;

			nRegister = AllocateRegister();
			assert(m_nRegisterLookup[nRegister] < 8); 

			LoadRelativeInRegister64(nRegister, nRelative1);

			//shl reg, cl
			m_pBlock->StreamWrite(3, 0x48, 0xD3, MakeRegFunRm(nRegister, 0x04));

			PushReg64(nRegister);
		}
		else
#endif
		{
			unsigned int nRegister1, nRegister2;
			unsigned int nDoneJmp[2];
			unsigned int nMoreJmp;

			nRegister1 = AllocateRegister();
			nRegister2 = AllocateRegister();

			LoadRelativeInRegister(nRegister1, nRelative1);
			LoadRelativeInRegister(nRegister2, nRelative2);

			//test cl, cl
			m_pBlock->StreamWrite(2, 0x84, 0xC9);

			//je $done
			m_pBlock->StreamWrite(2, 0x74, 0x00);
			nDoneJmp[0] = m_pBlock->StreamGetSize();

			//cmp cl, 0x20
			m_pBlock->StreamWrite(3, 0x80, 0xF9, 0x20);

			//jae $more32
			m_pBlock->StreamWrite(2, 0x73, 0x00);
			nMoreJmp = m_pBlock->StreamGetSize();

			//shld reg2, reg1, cl
			m_pBlock->StreamWrite(3, 0x0F, 0xA5, MakeRegRegRm(nRegister2, nRegister1));

			//shl reg1, cl
			m_pBlock->StreamWrite(2, 0xD3, MakeRegFunRm(nRegister1, 0x04));

			//jmp $done
			m_pBlock->StreamWrite(2, 0xEB, 0x00);
			nDoneJmp[1] = m_pBlock->StreamGetSize();

//$more32
			m_pBlock->StreamWriteAt(nMoreJmp - 1, m_pBlock->StreamGetSize() - nMoreJmp);

			//mov reg2, reg1
			m_pBlock->StreamWrite(2, 0x89, MakeRegRegRm(nRegister2, nRegister1));

			//xor reg1, reg1
			m_pBlock->StreamWrite(2, 0x33, MakeRegRegRm(nRegister1, nRegister1));

			//and cl, 0x1F
			m_pBlock->StreamWrite(3, 0x80, 0xE1, 0x1F);

			//shl reg2, cl
			m_pBlock->StreamWrite(2, 0xD3, MakeRegFunRm(nRegister2, 0x04));

//$done
			m_pBlock->StreamWriteAt(nDoneJmp[0] - 1, m_pBlock->StreamGetSize() - nDoneJmp[0]);
			m_pBlock->StreamWriteAt(nDoneJmp[1] - 1, m_pBlock->StreamGetSize() - nDoneJmp[1]);

			PushReg(nRegister1);
			PushReg(nRegister2);
		}

		FreeRegister(nAmountReg);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Shl64(uint8 nAmount)
{
	if(\
		(m_Shadow.GetAt(0) == RELATIVE) && \
		(m_Shadow.GetAt(2) == RELATIVE))
	{

		uint32 nRelative1, nRelative2;

		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		assert(nAmount < 0x40);

#ifdef AMD64
		
		if((nRelative2 - nRelative1) == 4)
		{
			unsigned int nRegister;

			nRegister = AllocateRegister();
			LoadRelativeInRegister64(nRegister, nRelative1);

			assert(m_nRegisterLookup[nRegister] < 8);

			//shl reg, amount
			m_pBlock->StreamWrite(4, 0x48, 0xC1, MakeRegFunRm(nRegister, 0x04), nAmount);

			PushReg64(nRegister);
		}
		else
#endif
		{
			if(nAmount == 0)
			{
				uint32 nRegister1, nRegister2;

				nRegister1 = AllocateRegister();
				nRegister2 = AllocateRegister();

				LoadRelativeInRegister(nRegister1, nRelative1);
				LoadRelativeInRegister(nRegister2, nRelative2);

				PushReg(nRegister1);
				PushReg(nRegister2);
			}
			else if(nAmount == 32)
			{
				uint32 nRegister;

				nRegister = AllocateRegister();
				LoadRelativeInRegister(nRegister, nRelative1);

				PushCst(0);
				PushReg(nRegister);
			}
			else if(nAmount > 32)
			{
				uint32 nRegister;

				nRegister = AllocateRegister();

				LoadRelativeInRegister(nRegister, nRelative1);

				//shl reg, amount
				m_pBlock->StreamWrite(3, 0xC1, MakeRegFunRm(nRegister, 0x04), nAmount & 0x1F);

				PushCst(0);
				PushReg(nRegister);
			}
			else
			{
				uint32 nRegister1, nRegister2;

				nRegister1 = AllocateRegister();
				nRegister2 = AllocateRegister();

				LoadRelativeInRegister(nRegister1, nRelative1);
				LoadRelativeInRegister(nRegister2, nRelative2);

				//shld nReg2, nReg1, nAmount
				m_pBlock->StreamWrite(4, 0x0F, 0xA4, MakeRegRegRm(nRegister2, nRegister1), nAmount);

				//shl nReg1, nAmount
				m_pBlock->StreamWrite(3, 0xC1, MakeRegFunRm(nRegister1, 0x04), nAmount);

				PushReg(nRegister1);
				PushReg(nRegister2);
			}
		}

	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Srl64()
{
	if(\
		(m_Shadow.GetAt(0) == RELATIVE) && \
		(m_Shadow.GetAt(2) == RELATIVE) && \
		(m_Shadow.GetAt(4) == RELATIVE))
	{
		uint32 nAmount, nRelative1, nRelative2;
		unsigned int nAmountReg;

		m_Shadow.Pull();
		nAmount = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		nAmountReg = AllocateRegister(REGISTER_SHIFTAMOUNT);
		LoadRelativeInRegister(nAmountReg, nAmount);

		//and cl, 0x3F
		m_pBlock->StreamWrite(3, 0x80, 0xC0 | (0x4 << 3) | 1, 0x3F);

#ifdef AMD64

		if((nRelative2 - nRelative1) == 4)
		{
			unsigned int nRegister;

			nRegister = AllocateRegister();
			assert(m_nRegisterLookup[nRegister] < 8); 

			LoadRelativeInRegister64(nRegister, nRelative1);

			//shr reg, cl
			m_pBlock->StreamWrite(3, 0x48, 0xD3, MakeRegFunRm(nRegister, 0x05));

			PushReg64(nRegister);
		}
		else
#endif
		{
			unsigned int nRegister1, nRegister2;
			unsigned int nDoneJmp[2];
			unsigned int nMoreJmp;

			nRegister1 = AllocateRegister();
			nRegister2 = AllocateRegister();

			LoadRelativeInRegister(nRegister1, nRelative1);
			LoadRelativeInRegister(nRegister2, nRelative2);

			//test cl, cl
			m_pBlock->StreamWrite(2, 0x84, 0xC9);

			//je $done
			m_pBlock->StreamWrite(2, 0x74, 0x00);
			nDoneJmp[0] = m_pBlock->StreamGetSize();

			//cmp cl, 0x20
			m_pBlock->StreamWrite(3, 0x80, 0xF9, 0x20);

			//jae $more32
			m_pBlock->StreamWrite(2, 0x73, 0x00);
			nMoreJmp = m_pBlock->StreamGetSize();

			//shrd reg1, reg2, cl
			m_pBlock->StreamWrite(3, 0x0F, 0xAD, MakeRegRegRm(nRegister1, nRegister2));

			//shr reg2, cl
			m_pBlock->StreamWrite(2, 0xD3, MakeRegFunRm(nRegister2, 0x05));

			//jmp $done
			m_pBlock->StreamWrite(2, 0xEB, 0x00);
			nDoneJmp[1] = m_pBlock->StreamGetSize();

//$more32
			m_pBlock->StreamWriteAt(nMoreJmp - 1, m_pBlock->StreamGetSize() - nMoreJmp);

			//mov reg1, reg2
			m_pBlock->StreamWrite(2, 0x89, MakeRegRegRm(nRegister1, nRegister2));

			//xor reg2, reg2
			m_pBlock->StreamWrite(2, 0x33, MakeRegRegRm(nRegister2, nRegister2));

			//and cl, 0x1F
			m_pBlock->StreamWrite(3, 0x80, 0xE1, 0x1F);

			//shr reg1, cl
			m_pBlock->StreamWrite(2, 0xD3, MakeRegFunRm(nRegister1, 0x05));

//$done
			m_pBlock->StreamWriteAt(nDoneJmp[0] - 1, m_pBlock->StreamGetSize() - nDoneJmp[0]);
			m_pBlock->StreamWriteAt(nDoneJmp[1] - 1, m_pBlock->StreamGetSize() - nDoneJmp[1]);

			PushReg(nRegister1);
			PushReg(nRegister2);
		}

		FreeRegister(nAmountReg);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Srl64(uint8 nAmount)
{
	if(\
		(m_Shadow.GetAt(0) == RELATIVE) && \
		(m_Shadow.GetAt(2) == RELATIVE))
	{
		uint32 nRelative1, nRelative2;

		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		assert(nAmount < 0x40);

		if(nAmount == 0)
		{
			uint32 nRegister1, nRegister2;

			nRegister1 = AllocateRegister();
			nRegister2 = AllocateRegister();

			LoadRelativeInRegister(nRegister1, nRelative1);
			LoadRelativeInRegister(nRegister2, nRelative2);

			PushReg(nRegister1);
			PushReg(nRegister2);
		}
		else if(nAmount == 32)
		{
			uint32 nRegister;

			nRegister = AllocateRegister();
			LoadRelativeInRegister(nRegister, nRelative2);

			PushReg(nRegister);
			PushCst(0);
		}
		else if(nAmount > 32)
		{
			uint32 nRegister;

			nRegister = AllocateRegister();

			LoadRelativeInRegister(nRegister, nRelative2);

			//shr reg, amount
			m_pBlock->StreamWrite(3, 0xC1, MakeRegFunRm(nRegister, 0x05), nAmount & 0x1F);

			PushReg(nRegister);
			PushCst(0);
		}
		else
		{
			uint32 nRegister1, nRegister2;

			nRegister1 = AllocateRegister();
			nRegister2 = AllocateRegister();

			LoadRelativeInRegister(nRegister1, nRelative1);
			LoadRelativeInRegister(nRegister2, nRelative2);

			//shrd nReg1, nReg2, nAmount
			m_pBlock->StreamWrite(4, 0x0F, 0xAC, MakeRegRegRm(nRegister1, nRegister2), nAmount);

			//shr nReg2, nAmount
			m_pBlock->StreamWrite(3, 0xC1, MakeRegFunRm(nRegister2, 0x05), nAmount);

			PushReg(nRegister1);
			PushReg(nRegister2);
		}
	}
}

void CCodeGen::Sub()
{
	if((m_Shadow.GetAt(0) == VARIABLE) && (m_Shadow.GetAt(2) == VARIABLE))
	{
		uint32 nVariable1, nVariable2, nRegister;

		m_Shadow.Pull();
		nVariable2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable1 = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable1);

		//sub reg, dword ptr[Variable2]
		m_pBlock->StreamWrite(2, 0x2B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Xor()
{
	if((m_Shadow.GetAt(0) == VARIABLE) && (m_Shadow.GetAt(2) == VARIABLE))
	{
		uint32 nVariable1, nVariable2, nRegister;

		m_Shadow.Pull();
		nVariable2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable1 = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable1);

		//xor reg, dword ptr[Variable2]
		m_pBlock->StreamWrite(2, 0x33, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if((m_Shadow.GetAt(0) == RELATIVE) && (m_Shadow.GetAt(2) == RELATIVE))
	{
		uint32 nRelative1, nRelative2, nRegister;

		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, nRelative1);

		//xor reg, dword ptr[base + rel]
		m_pBlock->StreamWrite(1, 0x33);
		WriteRelativeRmRegister(nRegister, nRelative2);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Cmp64Eq()
{
	if(\
		(m_Shadow.GetAt(0) == RELATIVE) && \
		(m_Shadow.GetAt(2) == RELATIVE) && \
		(m_Shadow.GetAt(4) == RELATIVE) && \
		(m_Shadow.GetAt(6) == RELATIVE))
	{
		uint32 nRelative1, nRelative2, nRelative3, nRelative4;
		uint32 nRegister1, nRegister2;

		m_Shadow.Pull();
		nRelative4 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative3 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		nRegister1 = AllocateRegister(REGISTER_HASLOW);
		nRegister2 = AllocateRegister(REGISTER_HASLOW);

#ifdef AMD64

		if(((nRelative4 - nRelative3) == 4) && ((nRelative2 - nRelative1) == 4))
		{
			LoadRelativeInRegister64(nRegister1, nRelative1);

			//cmp reg, qword ptr[base + rel4]
			m_pBlock->StreamWrite(2, 0x48, 0x3B);
			WriteRelativeRmRegister(nRegister1, nRelative3);

			//sete reg[l]
			m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister1]));

			//movzx reg, reg[l]
			m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister1]));
		}
		else

#endif
		{
			//mov nReg1, dword ptr[nRel2]
			LoadRelativeInRegister(nRegister1, nRelative2);

			//cmp nReg1, dword ptr[nRel4]
			m_pBlock->StreamWrite(1, 0x3B);
			WriteRelativeRmRegister(nRegister1, nRelative4);

			//sete reg1[l]
			m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister1]));

			//mov reg2, dword ptr[nRel1]
			LoadRelativeInRegister(nRegister2, nRelative1);

			//cmp reg2, dword ptr[nRel3]
			m_pBlock->StreamWrite(1, 0x3B);
			WriteRelativeRmRegister(nRegister2, nRelative3);

			//sete reg2[l]
			m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister2]));

			//and reg1, reg2
			m_pBlock->StreamWrite(2, 0x23, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister2]));

			//movzx reg1, reg1[l]
			m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister1]));

		}

		FreeRegister(nRegister2);

		m_Shadow.Push(nRegister1);
		m_Shadow.Push(REGISTER);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Cmp64Lt(bool nSigned)
{
	if(\
		(m_Shadow.GetAt(0) == VARIABLE) && \
		(m_Shadow.GetAt(2) == VARIABLE) && \
		(m_Shadow.GetAt(4) == VARIABLE) && \
		(m_Shadow.GetAt(6) == VARIABLE))
	{
		uint32 nVariable1, nVariable2, nVariable3, nVariable4;
		uint32 nRegister;

		m_Shadow.Pull();
		nVariable4 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable3 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable1 = m_Shadow.Pull();

		nRegister = AllocateRegister();

		/////////////////////////////////////////
		//Check high order word if equal

		//mov reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2);

		//cmp reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x3B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable4);

		//je +0x08
		m_pBlock->StreamWrite(2, 0x74, 0x08);

		///////////////////////////////////////////////////////////
		//If they aren't equal, next comparaison decides of result

		//setb/l reg[l]
		m_pBlock->StreamWrite(3, 0x0F, nSigned ? 0x9C : 0x92, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

		//movzx reg, reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

		//jmp +0x12
		m_pBlock->StreamWrite(2, 0xEB, 0x12);

		////////////////////////////////////////////////////////////
		//If they are equal, next comparaison decides of result

		//mov reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable1);

		//cmp reg, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x3B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable3);

		//setb reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x92, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

		//movzx reg, reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else if(\
		(m_Shadow.GetAt(0) == RELATIVE) && \
		(m_Shadow.GetAt(2) == RELATIVE) && \
		(m_Shadow.GetAt(4) == RELATIVE) && \
		(m_Shadow.GetAt(6) == RELATIVE))
	{
		uint32 nRelative1, nRelative2, nRelative3, nRelative4;
		uint32 nRegister;
		unsigned int nJmpPos1, nJmpPos2;

		m_Shadow.Pull();
		nRelative4 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative3 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		nRegister = AllocateRegister();
		assert(m_nRegisterLookup[nRegister] < 8);

#ifdef AMD64

		if(((nRelative4 - nRelative3) == 4) && ((nRelative2 - nRelative1) == 4))
		{
			LoadRelativeInRegister64(nRegister, nRelative1);

			//cmp reg, qword ptr[base + rel4]
			m_pBlock->StreamWrite(2, 0x48, 0x3B);
			WriteRelativeRmRegister(nRegister, nRelative3);

			//setb/l reg[l]
			m_pBlock->StreamWrite(3, 0x0F, nSigned ? 0x9C : 0x92, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

			//movzx reg, reg[l]
			m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		}
		else

#endif

		{
			/////////////////////////////////////////
			//Check high order word if equal

			//mov reg, dword ptr[base + rel2]
			LoadRelativeInRegister(nRegister, nRelative2);

			//cmp reg, dword ptr[base + rel4]
			m_pBlock->StreamWrite(1, 0x3B);
			WriteRelativeRmRegister(nRegister, nRelative4);

			//je +0x08
			m_pBlock->StreamWrite(2, 0x74, 0x08);

			///////////////////////////////////////////////////////////
			//If they aren't equal, this comparaison decides of result

			//setb/l reg[l]
			m_pBlock->StreamWrite(3, 0x0F, nSigned ? 0x9C : 0x92, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

			//movzx reg, reg[l]
			m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

			//jmp +?
			nJmpPos1 = m_pBlock->StreamGetSize() + 1;
			m_pBlock->StreamWrite(2, 0xEB, 0x00);

			////////////////////////////////////////////////////////////
			//If they are equal, next comparaison decides of result

			//mov reg, dword ptr[base + rel1]
			LoadRelativeInRegister(nRegister, nRelative1);

			//cmp reg, dword ptr[base + rel3]
			m_pBlock->StreamWrite(1, 0x3B);
			WriteRelativeRmRegister(nRegister, nRelative3);

			//setb reg[l]
			m_pBlock->StreamWrite(3, 0x0F, 0x92, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

			//movzx reg, reg[l]
			m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

			//Fix Jmp Offset
			nJmpPos2 = m_pBlock->StreamGetSize();
			m_pBlock->StreamWriteAt(nJmpPos1, (nJmpPos2 - nJmpPos1 - 1));
		}

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
	}
	else
	{
		assert(0);
	}
}

bool CCodeGen::IsTopRegCstPairCom()
{
	return ((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == REGISTER)) ||
		((m_Shadow.GetAt(0) == REGISTER) && (m_Shadow.GetAt(2) == CONSTANT));
}

bool CCodeGen::IsTopRegZeroPairCom()
{
	return ((m_Shadow.GetAt(0) == CONSTANT)&& (m_Shadow.GetAt(1) == 0) && (m_Shadow.GetAt(2) == REGISTER)) ||
		((m_Shadow.GetAt(0) == REGISTER) && (m_Shadow.GetAt(2) == CONSTANT) && (m_Shadow.GetAt(3) == 0));
}

void CCodeGen::GetRegCstPairCom(unsigned int* pReg, uint32* pCst)
{
	if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == REGISTER))
	{
		m_Shadow.Pull();
		(pCst != NULL) ? (*pCst) = m_Shadow.Pull() : m_Shadow.Pull();
		m_Shadow.Pull();
		(pReg != NULL) ? (*pReg) = m_Shadow.Pull() : m_Shadow.Pull();
	}
	else if((m_Shadow.GetAt(0) == REGISTER) && (m_Shadow.GetAt(2) == CONSTANT))
	{
		m_Shadow.Pull();
		(pReg != NULL) ? (*pReg) = m_Shadow.Pull() : m_Shadow.Pull();
		m_Shadow.Pull();
		(pCst != NULL) ? (*pCst) = m_Shadow.Pull() : m_Shadow.Pull();
	}
	else
	{
		assert(0);
	}
}
