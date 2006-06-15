#include <assert.h>
#include "CodeGen.h"
#include "CodeGen_VUI128.h"
#include "CodeGen_VUF128.h"
#include "CodeGen_FPU.h"
#include "PtrMacro.h"

bool				CCodeGen::m_nBlockStarted = false;
CCacheBlock*		CCodeGen::m_pBlock = NULL;
CArrayStack<uint32>	CCodeGen::m_Shadow;
CArrayStack<uint32>	CCodeGen::m_IfStack;

bool				CCodeGen::m_nRegisterAllocated[MAX_REGISTER];
unsigned int		CCodeGen::m_nRegisterLookup[MAX_REGISTER] =
{
	0,
	1,
	2,
	3,
	6,
	7,
};

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

void CCodeGen::PushReg(unsigned int nRegister)
{
	m_Shadow.Push(nRegister);
	m_Shadow.Push(REGISTER);
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
	else
	{
		assert(0);
	}
}

void CCodeGen::Call(void* pFunc, unsigned int nParamCount, bool nKeepRet)
{
	uint32 nCallRegister;

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
		else if(m_Shadow.GetAt(0) == REGISTER)
		{
			uint32 nRegister;

			m_Shadow.Pull();
			nRegister = m_Shadow.Pull();

			if(!RegisterHasNextUse(nRegister))
			{
				FreeRegister(nRegister);
			}

			//push reg
			m_pBlock->StreamWrite(1, 0x50 | m_nRegisterLookup[nRegister]);
		}
		else if((m_Shadow.GetAt(0) == REFERENCE) || (m_Shadow.GetAt(0) == CONSTANT))
		{
			uint32 nNumber;

			m_Shadow.Pull();
			nNumber = m_Shadow.Pull();

			//push nNumber
			m_pBlock->StreamWrite(1, 0x68);
			m_pBlock->StreamWriteWord(nNumber);
		}
		else
		{
			assert(0);
		}
	}

	nCallRegister = AllocateRegister();

	//mov reg, pFunc
	m_pBlock->StreamWrite(1, 0xB8 | (m_nRegisterLookup[nCallRegister]));
	m_pBlock->StreamWriteWord((uint32)((uint8*)pFunc - (uint8*)0));

	//call reg
	m_pBlock->StreamWrite(2, 0xFF, 0xD0 | (m_nRegisterLookup[nCallRegister]));

	FreeRegister(nCallRegister);

	if(nParamCount != 0)
	{
		//add esp, nParams * 4;
		m_pBlock->StreamWrite(3, 0x83, 0xC4, nParamCount * 4);
	}

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
	else if((m_Shadow.GetAt(2) == REGISTER) && (m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(1) == 0) && (nCondition == CONDITION_EQ))
	{
		uint32 nRegister;

		m_Shadow.Pull();
		m_Shadow.Pull();
		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		//test reg, reg
		m_pBlock->StreamWrite(2, 0x85, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		
		//sete reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

		//movzx reg, reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));
		
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
	else
	{
		assert(0);
	}
}

void CCodeGen::SeX16()
{
	if(m_Shadow.GetAt(0) == REGISTER)
	{
		uint32 nRegister;

		m_Shadow.Pull();
		nRegister = m_Shadow.Pull();

		if(nRegister != 0)
		{
			assert(0);
		}

		//cwde
		m_pBlock->StreamWrite(1, 0x98);

		m_Shadow.Push(0);
		m_Shadow.Push(REGISTER);
	}
	else if(m_Shadow.GetAt(0) == VARIABLE)
	{
		uint32 nVariable, nRegister;

		m_Shadow.Pull();
		nVariable = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadVariableInRegister(nRegister, nVariable);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);

		SeX16();
	}
	else
	{
		assert(0);
	}	
}

void CCodeGen::SeX()
{
	if(m_Shadow.GetAt(0) == REGISTER)
	{
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

		SeX();
	}
	else
	{
		assert(0);
	}
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
	else
	{
		assert(0);
	}
}

void CCodeGen::Shr64(uint8 nAmount)
{
	if(\
		(m_Shadow.GetAt(0) == VARIABLE) && \
		(m_Shadow.GetAt(2) == VARIABLE))
	{
		uint32 nVariable1, nVariable2;

		m_Shadow.Pull();
		nVariable2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nVariable1 = m_Shadow.Pull();

		assert(nAmount < 0x40);

		if(nAmount == 0)
		{
			uint32 nRegister1, nRegister2;

			nRegister1 = AllocateRegister();
			nRegister2 = AllocateRegister();

			LoadVariableInRegister(nRegister1, nVariable1);
			LoadVariableInRegister(nRegister2, nVariable2);

			m_Shadow.Push(nRegister1);
			m_Shadow.Push(REGISTER);
			
			m_Shadow.Push(nRegister2);
			m_Shadow.Push(REGISTER);
		}
		else if(nAmount == 32)
		{
			uint32 nRegister;

			nRegister = AllocateRegister();
			LoadVariableInRegister(nRegister, nVariable2);

			m_Shadow.Push(nRegister);
			m_Shadow.Push(REGISTER);

			m_Shadow.Push(0);
			m_Shadow.Push(CONSTANT);
		}
		else if(nAmount > 32)
		{
			uint32 nRegister;

			nRegister = AllocateRegister();

			LoadVariableInRegister(nRegister, nVariable2);

			//shr reg, amount
			m_pBlock->StreamWrite(3, 0xC1, 0xC0 | (0x05 << 3) | (nRegister), nAmount & 0x1F);

			m_Shadow.Push(nRegister);
			m_Shadow.Push(REGISTER);
			
			m_Shadow.Push(0);
			m_Shadow.Push(CONSTANT);
		}
		else
		{
			uint32 nRegister1, nRegister2;

			nRegister1 = AllocateRegister();
			nRegister2 = AllocateRegister();

			LoadVariableInRegister(nRegister1, nVariable1);
			LoadVariableInRegister(nRegister2, nVariable2);

			//shrd nReg1, nReg2, nAmount
			m_pBlock->StreamWrite(4, 0x0F, 0xAC, 0xC0 | (nRegister2 << 3) | (nRegister1), nAmount);

			//shr nReg2, nAmount
			m_pBlock->StreamWrite(3, 0xC1, 0xC0 | (0x05 << 3) | (nRegister2), nAmount);

			m_Shadow.Push(nRegister1);
			m_Shadow.Push(REGISTER);

			m_Shadow.Push(nRegister2);
			m_Shadow.Push(REGISTER);
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
	else
	{
		assert(0);
	}
}

void CCodeGen::Cmp64Eq()
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

		//mov reg1, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister1] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2);

		//cmp reg1, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x3B, 0x00 | (m_nRegisterLookup[nRegister1] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable4);

		//sete reg1[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister1]));

		//mov reg2, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister2] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable1);

		//cmp reg2, dword ptr[Variable]
		m_pBlock->StreamWrite(2, 0x3B, 0x00 | (m_nRegisterLookup[nRegister2] << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable3);

		//sete reg2[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister2]));

		//and reg1, reg2
		m_pBlock->StreamWrite(2, 0x23, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister2]));

		//movzx reg1, reg1[l]
		m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister1]));

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

void CCodeGen::GetRegCstPairCom(unsigned int* pReg, uint32* pCst)
{
	if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == REGISTER))
	{
		m_Shadow.Pull();
		if(pCst != NULL) (*pCst) = m_Shadow.Pull();
		m_Shadow.Pull();
		if(pReg != NULL) (*pReg) = m_Shadow.Pull();
	}
	else if((m_Shadow.GetAt(0) == REGISTER) && (m_Shadow.GetAt(2) == CONSTANT))
	{
		m_Shadow.Pull();
		if(pReg != NULL) (*pReg) = m_Shadow.Pull();
		m_Shadow.Pull();
		if(pCst != NULL) (*pCst) = m_Shadow.Pull();
	}
	else
	{
		assert(0);
	}
}
