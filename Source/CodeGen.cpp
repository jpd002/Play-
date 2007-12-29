#include <assert.h>
#include "CodeGen.h"
#include "CodeGen_VUI128.h"
#include "CodeGen_VUF128.h"
#include "CodeGen_FPU.h"
#include "PtrMacro.h"
#include "CodeGen_StackPatterns.h"
#include <boost/bind.hpp>

using namespace boost;
using namespace Framework;
using namespace std;

bool					CCodeGen::m_nBlockStarted = false;
CCacheBlock*			CCodeGen::m_pBlock = NULL;
CArrayStack<uint32>		CCodeGen::m_Shadow;
#ifdef AMD64
CArrayStack<uint32, 2>	CCodeGen::m_PullReg64Stack;
#endif
CArrayStack<uint32>		CCodeGen::m_IfStack;
CX86Assembler           CCodeGen::m_Assembler
                                            (
                                                &CCodeGen::StreamWriteByte, 
                                                &CCodeGen::StreamWriteAt,
                                                &CCodeGen::StreamTell
                                            );

bool                    CCodeGen::m_nRegisterAllocated[MAX_REGISTER];
bool                    CCodeGen::m_xmmRegisterAllocated[MAX_XMM_REGISTER];
CStream*                CCodeGen::m_stream = NULL;

CX86Assembler::REGISTER CCodeGen::g_nBaseRegister = CX86Assembler::rBP;

#ifdef AMD64

unsigned int            CCodeGen::m_nRegisterLookup[MAX_REGISTER] =
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

CX86Assembler::REGISTER CCodeGen::m_nRegisterLookupEx[MAX_REGISTER] =
{
    CX86Assembler::rAX,
	CX86Assembler::rCX,
	CX86Assembler::rDX,
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
	CX86Assembler::r8,
	CX86Assembler::r9,
	CX86Assembler::r10,
	CX86Assembler::r11,
	CX86Assembler::r12,
	CX86Assembler::r13,
	CX86Assembler::r14,
	CX86Assembler::r15,
};

#else

unsigned int            CCodeGen::m_nRegisterLookup[MAX_REGISTER] =
{
	0,
	1,
	2,
	3,
	6,
	7,
};

CX86Assembler::REGISTER CCodeGen::m_nRegisterLookupEx[MAX_REGISTER] =
{
    CX86Assembler::rAX,
	CX86Assembler::rCX,
	CX86Assembler::rDX,
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
};

#endif

CCodeGen::CCodeGen()
{

}

CCodeGen::~CCodeGen()
{
    
}

void CCodeGen::SetStream(CStream* stream)
{
    m_stream = stream;
}

void CCodeGen::Begin(CCacheBlock* pBlock)
{
	assert(m_nBlockStarted == false);
	m_nBlockStarted = true;

	CodeGen::CFPU::Begin(pBlock);
	CodeGen::CVUI128::Begin(pBlock);
	CodeGen::CVUF128::Begin(pBlock);

	m_Shadow.Reset();
	m_pBlock = pBlock;

	for(unsigned int i = 0; i < MAX_REGISTER; i++)
	{
		m_nRegisterAllocated[i] = false;		
	}

    for(unsigned int i = 0; i < MAX_XMM_REGISTER; i++)
    {
        m_xmmRegisterAllocated[i] = false;
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

bool CCodeGen::IsStackEmpty()
{
    return m_Shadow.GetCount() == 0;
}

void CCodeGen::EndQuota()
{
	m_Assembler.Ret();
}

void CCodeGen::BeginIf(bool nCondition)
{
	if(m_Shadow.GetAt(0) == REGISTER)
	{
		m_Shadow.Pull();
		uint32 nRegister = m_Shadow.Pull();

		//test reg, reg
        m_Assembler.TestEd(m_nRegisterLookupEx[nRegister],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]));

		if(!RegisterHasNextUse(nRegister))
		{
			FreeRegister(nRegister);
		}

        CX86Assembler::LABEL ifLabel = m_Assembler.CreateLabel();

		//jcc label
        if(nCondition)
        {
            m_Assembler.JeJb(ifLabel);
        }
        else
        {
            m_Assembler.JneJb(ifLabel);
        }

		m_IfStack.Push(ifLabel);
		m_IfStack.Push(IFBLOCK);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::EndIf()
{
	assert(m_IfStack.GetAt(0) == IFBLOCK);

	m_IfStack.Pull();
    CX86Assembler::LABEL ifLabel = static_cast<CX86Assembler::LABEL>(m_IfStack.Pull());

    m_Assembler.MarkLabel(ifLabel);
    m_Assembler.ResolveLabelReferences();
}

void CCodeGen::BeginIfElse(bool nCondition)
{
	if(FitsPattern<SingleRegister>())
	{
        SingleRegister::PatternValue valueRegister(GetPattern<SingleRegister>());

        if(!RegisterHasNextUse(valueRegister))
		{
			FreeRegister(valueRegister);
		}

        m_Assembler.TestEd(m_nRegisterLookupEx[valueRegister],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[valueRegister]));

        CX86Assembler::LABEL ifLabel = m_Assembler.CreateLabel();

        //jcc label
        if(nCondition)
        {
            m_Assembler.JeJb(ifLabel);
        }
        else
        {
            m_Assembler.JneJb(ifLabel);
        }

		m_IfStack.Push(ifLabel);
		m_IfStack.Push(IFELSEBLOCK);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::BeginIfElseAlt()
{
	assert(m_IfStack.GetAt(0) == IFELSEBLOCK);

	m_IfStack.Pull();
    CX86Assembler::LABEL ifLabel = m_IfStack.Pull();

    CX86Assembler::LABEL doneLabel = m_Assembler.CreateLabel();

    //jmp label
    m_Assembler.JmpJb(doneLabel);
    m_Assembler.MarkLabel(ifLabel);

	m_IfStack.Push(doneLabel);
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

unsigned int CCodeGen::GetMinimumConstantSize64(uint64 nConstant)
{
    if(((int64)nConstant >= -128) && ((int64)nConstant <= 127))
    {
        return 1;
    }
    if(((int64)nConstant >= -2147483647) && ((int64)nConstant <= 2147483647))
    {
        return 4;
    }
    return 8;
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

CCodeGen::XMMREGISTER CCodeGen::AllocateXmmRegister()
{
    for(unsigned int i = 0; i < MAX_XMM_REGISTER; i++)
    {
        if(!m_xmmRegisterAllocated[i])
        {
            m_xmmRegisterAllocated[i] = true;
            return static_cast<XMMREGISTER>(i);
        }
    }

    throw runtime_error("All registers exhausted.");
}

void CCodeGen::FreeXmmRegister(XMMREGISTER registerId)
{
    m_xmmRegisterAllocated[registerId] = false;
}

void CCodeGen::LoadVariableInRegister(unsigned int nRegister, uint32 nVariable)
{
	//mov reg, dword ptr[Variable]
	m_pBlock->StreamWrite(2, 0x8B, 0x00 | (m_nRegisterLookup[nRegister] << 3) | (0x05));
	m_pBlock->StreamWriteWord(nVariable);
}

void CCodeGen::LoadRelativeInRegister(unsigned int nRegister, uint32 nOffset)
{
	//mov reg, [base + rel]
    m_Assembler.MovEd(
        m_nRegisterLookupEx[nRegister],
        CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, nOffset));
}

void CCodeGen::LoadConstantInRegister(unsigned int nRegister, uint32 nConstant)
{
    if(nConstant == 0)
    {
        m_Assembler.XorGd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]),
            m_nRegisterLookupEx[nRegister]);
    }
    else
    {
        m_Assembler.MovId(m_nRegisterLookupEx[nRegister], nConstant);
    }
}

void CCodeGen::CopyRegister(unsigned int nDst, unsigned int nSrc)
{
    m_Assembler.MovEd(
        m_nRegisterLookupEx[nDst],
        CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nSrc]));
}

#ifdef AMD64

void CCodeGen::LoadRelativeInRegister64(unsigned int nRegister, uint32 nRelative)
{
    m_Assembler.MovEq(
        m_nRegisterLookupEx[nRegister],
        CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, nRelative));
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

bool CCodeGen::IsRegisterSaved(unsigned int registerId)
{
#ifdef AMD64
    throw runtime_error("Not implemented.");
#else
    CX86Assembler::REGISTER registerName = m_nRegisterLookupEx[registerId];
    return
        (
        registerName == CX86Assembler::rBX || 
        registerName == CX86Assembler::rBP ||
        registerName == CX86Assembler::rSP ||
        registerName == CX86Assembler::rDI ||
        registerName == CX86Assembler::rSI
        );
#endif
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
        m_Assembler.MovGd(
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(nOffset)),
            m_nRegisterLookupEx[nRegister]);

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
            m_Assembler.XorGd(
                CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]),
                m_nRegisterLookupEx[nRegister]);
		}
		else
		{
			//mov reg, $Constant
            m_Assembler.MovId(m_nRegisterLookupEx[nRegister], nConstant);
		}

		//mov dword ptr[pValue], reg
        m_Assembler.MovGd(
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(nOffset)),
            m_nRegisterLookupEx[nRegister]);

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
    uint32 type = m_Shadow.Pull();
    uint32 value = m_Shadow.Pull();
    if(type == REGISTER)
    {
        if(!RegisterHasNextUse(value))
        {
            FreeRegister(value);
        }
    }
}

void CCodeGen::Add()
{
	if(FitsPattern<CommutativeRelativeConstant>())
	{
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());

		unsigned int registerId = AllocateRegister();

		LoadRelativeInRegister(registerId, ops.first);

        //add reg, Immediate
        m_Assembler.AddId(
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[registerId]),
            ops.second);

        PushReg(registerId);
	}
	else if(FitsPattern<RelativeRelative>())
	{
        RelativeRelative::PatternValue ops = GetPattern<RelativeRelative>();
        unsigned int resultRegister = AllocateRegister();

        LoadRelativeInRegister(resultRegister, ops.first);
        m_Assembler.AddEd(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));

        PushReg(resultRegister);
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

        m_Assembler.AddId(
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]),
            nConstant);

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
/*
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

		nRegister1 = AllocateRegister();
		nRegister2 = AllocateRegister();

		LoadRelativeInRegister(nRegister1, nRelative1);
		LoadRelativeInRegister(nRegister2, nRelative2);

		//add reg1, dword ptr[Relative3]
		m_pBlock->StreamWrite(1, 0x03);
		WriteRelativeRmRegister(nRegister1, nRelative3);

		//adc reg2, dword ptr[Relative4]
		m_pBlock->StreamWrite(1, 0x13);
		WriteRelativeRmRegister(nRegister2, nRelative4);

		PushReg(nRegister1);
		PushReg(nRegister2);
	}
    else if(\
		(m_Shadow.GetAt(0) == CONSTANT) && \
		(m_Shadow.GetAt(2) == CONSTANT) && \
		(m_Shadow.GetAt(4) == RELATIVE) && \
		(m_Shadow.GetAt(6) == RELATIVE))
    {
		uint32 nRelative1, nRelative2;
        uint32 nConstant1, nConstant2;
		uint32 nRegister1, nRegister2;

		m_Shadow.Pull();
		nConstant2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nConstant1 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		nRegister1 = AllocateRegister();
		nRegister2 = AllocateRegister();

		LoadRelativeInRegister(nRegister1, nRelative1);
		LoadRelativeInRegister(nRegister2, nRelative2);

        //add reg1, const1
        X86_RegImmOp(nRegister1, nConstant1, 0x00);

        //adc reg2, const2
        X86_RegImmOp(nRegister2, nConstant2, 0x02);

		PushReg(nRegister1);
		PushReg(nRegister2);
    }
	else
	{
		assert(0);
	}
*/

    if(FitsPattern<ConstantConstant64>())
    {
        ConstantConstant64::PatternValue ops(GetPattern<ConstantConstant64>());
        uint64 result = ops.second.q + ops.first.q;
        
        PushCst(static_cast<uint32>(result));
        PushCst(static_cast<uint32>(result >> 32));
    }
    else if(FitsPattern<CommutativeRelativeConstant64>())
    {
        CommutativeRelativeConstant64::PatternValue ops(GetPattern<CommutativeRelativeConstant64>());

        if(ops.second.q == 0)
        {
            //Simple copy
            PushRel(ops.first.d0);
            PushRel(ops.first.d1);
        }
        else
        {
			unsigned int resultLow = AllocateRegister();
			unsigned int resultHigh = AllocateRegister();
			
			LoadRelativeInRegister(resultLow, ops.first.d0);
			LoadRelativeInRegister(resultHigh, ops.first.d1);
			
			m_Assembler.AddId(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]),
				ops.second.d0);
			m_Assembler.AdcId(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultHigh]),
				ops.second.d1);
				
			PushReg(resultLow);
			PushReg(resultHigh);
        }
    }
    else if(FitsPattern<ZeroWithSomethingCommutative64>())
    {
        ZeroWithSomethingCommutative64::PatternValue ops(GetPattern<ZeroWithSomethingCommutative64>());
        //We have something like : op + 0
        //We just need to copy it over
        m_Shadow.Push(ops.first.d0);
        m_Shadow.Push(ops.first.d1);
        m_Shadow.Push(ops.second.d0);
        m_Shadow.Push(ops.second.d1);
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
	else if(FitsPattern<CommutativeRegisterConstant>())
	{
        CommutativeRegisterConstant::PatternValue ops(GetPattern<CommutativeRegisterConstant>());

        m_Assembler.AndId(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[ops.first]), ops.second);

        PushReg(ops.first);
	}
	else if(FitsPattern<CommutativeRelativeConstant>())
	{
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());

		unsigned int nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, ops.first);

        PushReg(nRegister);
        PushCst(ops.second);

        And();
	}
	else if(FitsPattern<ConstantConstant>())
	{
        ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
		PushCst(ops.first & ops.second);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::And64()
{
	if(FitsPattern<RelativeRelative64>())
    {
        RelativeRelative64::PatternValue ops(GetPattern<RelativeRelative64>());

		uint32 nRegister1 = AllocateRegister();
		uint32 nRegister2 = AllocateRegister();

		LoadRelativeInRegister(nRegister1, ops.first.d0);
		LoadRelativeInRegister(nRegister2, ops.first.d1);

        m_Assembler.AndEd(m_nRegisterLookupEx[nRegister1],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second.d0));

        m_Assembler.AndEd(m_nRegisterLookupEx[nRegister2],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second.d1));

		PushReg(nRegister1);
		PushReg(nRegister2);
	}
    else if(FitsPattern<CommutativeRelativeConstant64>())
    {
        CommutativeRelativeConstant64::PatternValue ops(GetPattern<CommutativeRelativeConstant64>());

        for(unsigned int i = 0; i < 2; i++)
        {
            if(ops.second.d[i] == 0)
            {
                //x & 0 = 0
                PushCst(0);
                continue;
            }
            unsigned int registerId = AllocateRegister();
            LoadRelativeInRegister(registerId, ops.first.d[i]);
            m_Assembler.AndId(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[registerId]), ops.second.d[i]);
            PushReg(registerId);
        }
    }
	else
	{
		assert(0);
	}
}

void CCodeGen::Call(void* pFunc, unsigned int nParamCount, bool nKeepRet)
{
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
            m_Assembler.PushEd(
                CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, nRelative));

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
            m_Assembler.Push(m_nRegisterLookupEx[nRegister]);

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
            m_Assembler.PushId(nNumber);

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

	unsigned int nCallRegister = AllocateRegister();

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
    m_Assembler.MovId(
        m_nRegisterLookupEx[nCallRegister],
        static_cast<uint32>(reinterpret_cast<uint8*>(pFunc) - reinterpret_cast<uint8*>(NULL)));

    //call reg
    m_Assembler.CallEd(
        CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nCallRegister]));


    if(nParamCount != 0)
	{
		//add esp, nParams * 4;
        m_Assembler.AddId(
            CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), nParamCount * 4);
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
    if(
        nCondition == CONDITION_EQ &&
        FitsPattern<CommutativeRelativeConstant>()
        )
    {
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());
        unsigned int resultRegister = AllocateRegister(REGISTER_HASLOW);

        //cmp [relative], $constant
        m_Assembler.CmpId(
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first),
            ops.second);

        //sete reg[l]
        m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));

		//movzx reg, reg[l]
        m_Assembler.MovzxEb(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));

        PushReg(resultRegister);
    }
    else if(
        nCondition == CONDITION_EQ &&
        FitsPattern<ConstantConstant>()
        )
    {
        ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
        unsigned int resultRegister = AllocateRegister();
        LoadConstantInRegister(resultRegister, ops.first == ops.second ? 1 : 0);
        PushReg(resultRegister);
    }
	else if(IsTopRegZeroPairCom() && (nCondition == CONDITION_EQ))
	{
		unsigned int valueRegister;

		GetRegCstPairCom(&valueRegister, NULL);

        unsigned int resultRegister = AllocateRegister(REGISTER_HASLOW);

		//test reg, reg
        m_Assembler.TestEd(m_nRegisterLookupEx[valueRegister],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[valueRegister]));
		
		//sete reg[l]
        m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));

		//movzx reg, reg[l]
        m_Assembler.MovzxEb(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));
		
        PushReg(resultRegister);
	}
/*
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
*/
	else
	{
		assert(0);
	}
}

void CCodeGen::Cmp64(CONDITION nCondition)
{
#ifdef AMD64
    if(IsTopContRelPair64() || IsTopContRelCstPair64())
    {
        //Can be done easily using 64-bits regs
        Cmp64Cont(nCondition);
    }
    else
#endif
    {
	    switch(nCondition)
	    {
	    case CONDITION_BL:
	    case CONDITION_LT:
		    Cmp64Lt(nCondition == CONDITION_LT, false);
		    break;
        case CONDITION_LE:
            Cmp64Lt(nCondition == CONDITION_LE, true);
            break;
	    case CONDITION_EQ:
		    Cmp64Eq();
		    break;
	    default:
		    assert(0);
		    break;
	    }
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

void CCodeGen::MultS()
{
    if(FitsPattern<CommutativeRelativeConstant>())
    {
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());

        //We need eax and edx for this
        assert(!m_nRegisterAllocated[REGISTER_EAX] && !m_nRegisterAllocated[REGISTER_EDX]);
        m_nRegisterAllocated[REGISTER_EAX] = true;
        m_nRegisterAllocated[REGISTER_EDX] = true;
        unsigned int lowRegister = REGISTER_EAX;
        unsigned int highRegister = REGISTER_EDX;

        LoadRelativeInRegister(lowRegister, ops.first);
        LoadConstantInRegister(highRegister, ops.second);
        m_Assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[highRegister]));

        PushReg(highRegister);
        PushReg(lowRegister);
    }
    else if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());

        //We need eax and edx for this
        assert(!m_nRegisterAllocated[REGISTER_EAX] && !m_nRegisterAllocated[REGISTER_EDX]);
        m_nRegisterAllocated[REGISTER_EAX] = true;
        m_nRegisterAllocated[REGISTER_EDX] = true;
        unsigned int lowRegister = REGISTER_EAX;
        unsigned int highRegister = REGISTER_EDX;

        LoadRelativeInRegister(lowRegister, ops.first);
        m_Assembler.ImulEd(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));

        PushReg(highRegister);
        PushReg(lowRegister);
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
    else if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());

        unsigned int nRegister = AllocateRegister();
        LoadRelativeInRegister(nRegister, ops.first);

        m_Assembler.OrEd(m_nRegisterLookupEx[nRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));

        PushReg(nRegister);
    }
    else if(FitsPattern<ConstantConstant>())
    {
        ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
        PushCst(ops.first | ops.second);
    }
    else if(FitsPattern<CommutativeRegisterConstant>())
	{
        CommutativeRegisterConstant::PatternValue ops(GetPattern<CommutativeRegisterConstant>());

		//or reg, const
        m_Assembler.OrId(
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[ops.first]),
            ops.second);

        PushReg(ops.first);
	}
	else if(FitsPattern<CommutativeRelativeConstant>())
	{
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());

		unsigned int nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, ops.first);

		PushReg(nRegister);
		PushCst(ops.second);

		Or();
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::SeX()
{
    if(m_Shadow.GetAt(0) == CONSTANT)
    {
        uint32 constant = m_Shadow.GetAt(1);

        //Sign extend the constant and push it on the stack
        PushCst(constant & 0x80000000 ? 0xFFFFFFFF : 0x00000000);
    }
    else
    {
	    ReduceToRegister();

	    m_Shadow.Pull();
	    unsigned int valueRegister = m_Shadow.Pull();

        unsigned int resultRegister = AllocateRegister();
        CopyRegister(resultRegister, valueRegister);

        m_Assembler.SarEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]),
            0x1F);

        PushReg(valueRegister);
        PushReg(resultRegister);
    }
}

void CCodeGen::SeX8()
{
    if(FitsPattern<SingleRegister>())
    {
        SingleRegister::PatternValue registerId(GetPattern<SingleRegister>());
        assert(!RegisterHasNextUse(registerId));
        m_Assembler.MovsxEb(m_nRegisterLookupEx[registerId],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[registerId]));
        PushReg(registerId);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::SeX16()
{
    ReduceToRegister();

    m_Shadow.Pull();
    uint32 nRegister = m_Shadow.Pull();

    m_Assembler.MovsxEw(m_nRegisterLookupEx[nRegister],
        CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]));

    PushReg(nRegister);
}

void CCodeGen::Shl(uint8 nAmount)
{
	if(FitsPattern<SingleRegister>())
	{
        SingleRegister::PatternValue op = GetPattern<SingleRegister>();
        m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[op]), nAmount);
        PushReg(op);
	}
	else if(FitsPattern<SingleRelative>())
	{
        UnaryRelativeSelfCallAsRegister(bind(&CCodeGen::Shl, nAmount));
	}
	else if(FitsPattern<SingleConstant>())
	{
        SingleConstant::PatternValue op = GetPattern<SingleConstant>();
		PushCst(op << nAmount);
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
    nAmount &= 0x3F;

	if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());
		uint32 nRelative1 = ops.first;
        uint32 nRelative2 = ops.second;

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
                m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]), nAmount & 0x1F);

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
                m_Assembler.ShldEd(
                    CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister2]),
                    m_nRegisterLookupEx[nRegister1], nAmount);

				//shl nReg1, nAmount
                m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister1]), nAmount);

				PushReg(nRegister1);
				PushReg(nRegister2);
			}
		}

	}
    else if(FitsPattern<RelativeConstant>())
    {
        RelativeConstant::PatternValue ops(GetPattern<RelativeConstant>());

        if(nAmount < 32)
        {
            unsigned int register1 = AllocateRegister();
            unsigned int register2 = AllocateRegister();

            LoadRelativeInRegister(register1, ops.first);
            LoadConstantInRegister(register2, ops.second);

            m_Assembler.ShldEd(
                CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[register2]),
                m_nRegisterLookupEx[register1], nAmount);

            m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[register1]), nAmount);

            PushReg(register1);
            PushReg(register2);
        } 
        else if(nAmount == 32)
        {
            PushCst(0);
            PushRel(ops.first);
        }
        else if(nAmount > 32)
        {
            unsigned int resultRegister = AllocateRegister();
            LoadRelativeInRegister(resultRegister, ops.first);
            m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]), nAmount - 32);
            PushCst(0);
            PushReg(resultRegister);
        }
        else
        {
            assert(0);
        }
    }
    else if(FitsPattern<SingleConstant64>())
    {
        SingleConstant64::PatternValue ops(GetPattern<SingleConstant64>());
        ops.q <<= nAmount;
        PushCst(ops.d0);
        PushCst(ops.d1);
    }
	else
	{
		assert(0);
	}
}

void CCodeGen::Sra(uint8 nAmount)
{
	if(FitsPattern<SingleRegister>())
	{
        SingleRegister::PatternValue registerId(GetPattern<SingleRegister>());
        m_Assembler.SarEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[registerId]), nAmount);
        PushReg(registerId);
	}
	else if(FitsPattern<SingleRelative>())
	{
        UnaryRelativeSelfCallAsRegister(bind(&CCodeGen::Sra, nAmount));
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Sra64(uint8 nAmount)
{
    if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());

        nAmount &= 0x3F;

        if(nAmount == 32)
        {
            PushRel(ops.second);
            SeX();
        }
        else if(nAmount > 32)
        {
            PushRel(ops.second);
            Sra(nAmount - 32);
            SeX();
        }
        else
        {
            assert(0);
        }
    }
    else if(FitsPattern<ConstantRelative>())
    {
        ConstantRelative::PatternValue ops(GetPattern<ConstantRelative>());

        nAmount &= 0x3F;
        if(nAmount == 32)
        {
            PushRel(ops.second);
            SeX();
        }
        else if(nAmount > 32)
        {
            PushRel(ops.second);
            Sra(nAmount - 32);
            SeX();
        }
        else
        {
            assert(0);
        }
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::Srl(uint8 nAmount)
{
	if(FitsPattern<SingleRegister>())
	{
        SingleRegister::PatternValue registerId = GetPattern<SingleRegister>();

        //shr nRegister, nAmount
        m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[registerId]), nAmount);

        PushReg(registerId);
	}
	else if(FitsPattern<SingleRelative>())
	{
        UnaryRelativeSelfCallAsRegister(bind(&CCodeGen::Srl, nAmount));
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
	if(FitsPattern<RelativeRelative>())
	{
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());
		uint32 nRelative1 = ops.first;
        uint32 nRelative2 = ops.second;

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
			uint32 nRegister = AllocateRegister();
			
			LoadRelativeInRegister(nRegister, nRelative2);

			//shr reg, amount
			m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]),
				nAmount & 0x1F);

			PushReg(nRegister);
			PushCst(0);
		}
		else
		{
			unsigned int nRegister1 = AllocateRegister();
			unsigned int nRegister2 = AllocateRegister();

			LoadRelativeInRegister(nRegister1, nRelative1);
			LoadRelativeInRegister(nRegister2, nRelative2);

			//shrd nReg1, nReg2, nAmount
            m_Assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister1]),
                m_nRegisterLookupEx[nRegister2],
                nAmount);

			//shr nReg2, nAmount
            m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister2]), nAmount);

			PushReg(nRegister1);
			PushReg(nRegister2);
		}
	}
    else if(FitsPattern<ConstantRelative>())
    {
        //HI = Relative
        //LO = Constant
        ConstantRelative::PatternValue ops(GetPattern<ConstantRelative>());

        if(nAmount == 32)
        {
            unsigned int resultRegister = AllocateRegister();
            LoadRelativeInRegister(resultRegister, ops.second);
            PushReg(resultRegister);
            PushCst(0);
		}
		else if(nAmount < 32)
		{
			unsigned int register1 = AllocateRegister();
			unsigned int register2 = AllocateRegister();

			LoadConstantInRegister(register1, ops.first);
			LoadRelativeInRegister(register2, ops.second);

			//shrd nReg1, nReg2, nAmount
            m_Assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[register1]),
                m_nRegisterLookupEx[register2],
                nAmount);

			//shr nReg2, nAmount
            m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[register2]), nAmount);

			PushReg(register1);
			PushReg(register2);		
        }
        else
        {
            assert(0);
        }
    }
	else if(FitsPattern<SingleConstant64>())
	{
        SingleConstant64::PatternValue ops(GetPattern<SingleConstant64>());
        ops.q >>= nAmount;
        PushCst(ops.d0);
        PushCst(ops.d1);
	}
    else
    {
        assert(0);
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
    else if((m_Shadow.GetAt(0) == RELATIVE) && (m_Shadow.GetAt(2) == RELATIVE))
    {
		uint32 nRelative1, nRelative2, nRegister;

		m_Shadow.Pull();
		nRelative2 = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative1 = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, nRelative1);

        m_Assembler.SubEd(
            m_nRegisterLookupEx[nRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, nRelative2));

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
    }
    else if((m_Shadow.GetAt(0) == CONSTANT) && (m_Shadow.GetAt(2) == RELATIVE))
    {
		uint32 nRelative, nConstant, nRegister;

		m_Shadow.Pull();
		nConstant = m_Shadow.Pull();
		m_Shadow.Pull();
		nRelative = m_Shadow.Pull();

		nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, nRelative);

        m_Assembler.SubId(
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]),
            nConstant);

		m_Shadow.Push(nRegister);
		m_Shadow.Push(REGISTER);
    }
    else if(FitsPattern<ConstantRelative>())
    {
        ConstantRelative::PatternValue ops(GetPattern<ConstantRelative>());
        unsigned int resultRegister = AllocateRegister();
        LoadConstantInRegister(resultRegister, ops.first);
        m_Assembler.SubEd(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        PushReg(resultRegister);
    }
	else
	{
		assert(0);
	}
}

void CCodeGen::Xor()
{
	if(FitsPattern<RelativeRelative>())
	{
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());

        unsigned int nRegister = AllocateRegister();
		LoadRelativeInRegister(nRegister, ops.first);

        m_Assembler.XorEd(m_nRegisterLookupEx[nRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));

        PushReg(nRegister);
	}
	else if(FitsPattern<CommutativeRelativeConstant>())
	{
		CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());
		
		unsigned int resultRegister = AllocateRegister();
		LoadRelativeInRegister(resultRegister, ops.first);
		
		m_Assembler.XorId(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]),
			ops.second);
		
		PushReg(resultRegister);
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Cmp64Eq()
{
    if(
        m_Shadow.GetAt(4) == CONSTANT &&
        m_Shadow.GetAt(0) == CONSTANT &&
        m_Shadow.GetAt(1) == m_Shadow.GetAt(5)
        )
    {
        m_Shadow.Pull();
        m_Shadow.Pull();
        uint32 type1 = m_Shadow.Pull();
        uint32 value1 = m_Shadow.Pull();
        m_Shadow.Pull();
        m_Shadow.Pull();
        uint32 type0 = m_Shadow.Pull();
        uint32 value0 = m_Shadow.Pull();

        m_Shadow.Push(value0);
        m_Shadow.Push(type0);
        m_Shadow.Push(value1);
        m_Shadow.Push(type1);

        Cmp(CONDITION_EQ);
    }
	else if(FitsPattern<RelativeRelative64>())
	{
		RelativeRelative64::PatternValue ops(GetPattern<RelativeRelative64>());

		uint32 nRegister1 = AllocateRegister(REGISTER_HASLOW);
		uint32 nRegister2 = AllocateRegister(REGISTER_HASLOW);

		LoadRelativeInRegister(nRegister1, ops.first.d1);
		m_Assembler.CmpEd(m_nRegisterLookupEx[nRegister1], 
			CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second.d1));
		m_Assembler.SeteEb(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister1]));
		
		LoadRelativeInRegister(nRegister2, ops.first.d0);
		m_Assembler.CmpEd(m_nRegisterLookupEx[nRegister2],
			CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second.d0));
		m_Assembler.SeteEb(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister2]));

		m_Assembler.AndEd(m_nRegisterLookupEx[nRegister1],
			CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister2]));
		m_Assembler.MovzxEb(m_nRegisterLookupEx[nRegister1], 
			CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister1]));

		FreeRegister(nRegister2);

		m_Shadow.Push(nRegister1);
		m_Shadow.Push(REGISTER);

	}
    else if(FitsPattern<CommutativeRelativeConstant64>())
    {
        CommutativeRelativeConstant64::PatternValue ops = GetPattern<CommutativeRelativeConstant64>();

        unsigned int resultRegister = AllocateRegister(REGISTER_HASLOW);
        unsigned int tempRegister = AllocateRegister(REGISTER_HASLOW);

        m_Assembler.CmpId(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first.d1),
            ops.second.d1);
        m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        m_Assembler.CmpId(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first.d0),
            ops.second.d0);
        m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[tempRegister]));
        m_Assembler.AndEd(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[tempRegister]));
        m_Assembler.MovzxEb(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));

        FreeRegister(tempRegister);
        PushReg(resultRegister);
    }
	else
	{
		assert(0);
	}
}

void CCodeGen::Cmp64Lt(bool nSigned, bool nOrEqual)
{
    struct EmitComparaison
    {
        void operator ()(uint32 nValueType, uint32 nValue, unsigned int nRegister, CX86Assembler& assembler)
        {
            switch(nValueType)
            {
            case CONSTANT:
			    //cmp reg, Immediate
                assembler.CmpId(
                    CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]),
                    nValue);
                break;
            case RELATIVE:
			    //cmp reg, dword ptr[rel]
                assembler.CmpEd(
                    m_nRegisterLookupEx[nRegister],
                    CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, nValue));
                break;
            default:
                assert(0);
                break;
            }
        }
    };

    struct EmitLoad
    {
        void operator ()(uint32 valueType, uint32 value, unsigned int registerId, CX86Assembler& assembler)
        {
            switch(valueType)
            {
            case CONSTANT:
                LoadConstantInRegister(registerId, value);
                break;
            case RELATIVE:
                LoadRelativeInRegister(registerId, value);
                break;
            default:
                assert(0);
                break;
            }
        }
    };

    uint32 nValue[4];
    uint32 nValueType[4];

    for(int i = 3; i >= 0; i--)
    {
        nValueType[i]   = m_Shadow.Pull();
        nValue[i]       = m_Shadow.Pull();
    }

	uint32 nRegister = AllocateRegister();

	{
        CX86Assembler::LABEL highOrderEqualLabel = m_Assembler.CreateLabel();
        CX86Assembler::LABEL doneLabel = m_Assembler.CreateLabel();

        /////////////////////////////////////////
		//Check high order word if equal

		//mov reg, dword ptr[base + rel2]
        EmitLoad()(nValueType[1], nValue[1], nRegister, m_Assembler);
        EmitComparaison()(nValueType[3], nValue[3], nRegister, m_Assembler);

        //je highOrderEqual
        m_Assembler.JeJb(highOrderEqualLabel);

		///////////////////////////////////////////////////////////
		//If they aren't equal, this comparaison decides of result

		//setb/l reg[l]
        if(nSigned)
        {
            m_Assembler.SetlEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[nRegister]));
        }
        else
        {
            m_Assembler.SetbEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[nRegister]));
        }

		//movzx reg, reg[l]
        m_Assembler.MovzxEb(
            m_nRegisterLookupEx[nRegister],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[nRegister]));

		//jmp done
        m_Assembler.JmpJb(doneLabel);

		//highOrderEqual: /////////////////////////////////////
        m_Assembler.MarkLabel(highOrderEqualLabel);
        //If they are equal, next comparaison decides of result

        EmitLoad()(nValueType[0], nValue[0], nRegister, m_Assembler);
        EmitComparaison()(nValueType[2], nValue[2], nRegister, m_Assembler);

	    //setb/be reg[l]
        if(nOrEqual)
        {
            m_Assembler.SetbeEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[nRegister]));
        }
        else
        {
            m_Assembler.SetbEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[nRegister]));
        }

		//movzx reg, reg[l]
        m_Assembler.MovzxEb(
            m_nRegisterLookupEx[nRegister],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[nRegister]));

        //done: ///////////////////////////////////////////////
        m_Assembler.MarkLabel(doneLabel);

        m_Assembler.ResolveLabelReferences();
	}

	m_Shadow.Push(nRegister);
	m_Shadow.Push(REGISTER);
}

void CCodeGen::Cmp64Cont(CONDITION nCondition)
{
#ifdef AMD64
	uint32 nRegister;

    nRegister = AllocateRegister(REGISTER_HASLOW);

    if(
        (m_Shadow.GetAt(0) == RELATIVE) &&
        (m_Shadow.GetAt(2) == RELATIVE) &&
        (m_Shadow.GetAt(4) == RELATIVE) &&
        (m_Shadow.GetAt(6) == RELATIVE)
        )
    {
        uint32 nRelative1, nRelative2, nRelative3, nRelative4;

	    m_Shadow.Pull();
	    nRelative4 = m_Shadow.Pull();
	    m_Shadow.Pull();
	    nRelative3 = m_Shadow.Pull();
	    m_Shadow.Pull();
	    nRelative2 = m_Shadow.Pull();
	    m_Shadow.Pull();
	    nRelative1 = m_Shadow.Pull();

        //Emit the comparason instruction
		LoadRelativeInRegister64(nRegister, nRelative1);

		//cmp reg, qword ptr[base + rel4]
        m_Assembler.CmpEq(
            m_nRegisterLookupEx[nRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, nRelative3));
    }
    else if(
        (m_Shadow.GetAt(0) == CONSTANT) &&
        (m_Shadow.GetAt(2) == CONSTANT) &&
        (m_Shadow.GetAt(4) == RELATIVE) &&
        (m_Shadow.GetAt(6) == RELATIVE)
        )
    {
        uint64 nConstant;
        uint64 nConstant1, nConstant2;
        uint32 nRelative1, nRelative2;

	    m_Shadow.Pull();
	    nConstant2 = m_Shadow.Pull();
	    m_Shadow.Pull();
	    nConstant1 = m_Shadow.Pull();
	    m_Shadow.Pull();
	    nRelative2 = m_Shadow.Pull();
	    m_Shadow.Pull();
	    nRelative1 = m_Shadow.Pull();

        LoadRelativeInRegister64(nRegister, nRelative1);

        nConstant = (nConstant2 << 32) | (nConstant1);

        m_Assembler.CmpIq(
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]),
            nConstant);
    }
    else
    {
        assert(0);
    }

    uint8 nInstruction;

    switch(nCondition)
    {
    case CONDITION_EQ:
        nInstruction = 0x94;
        break;
    case CONDITION_LT:
        nInstruction = 0x9C;
        break;
    case CONDITION_BL:
        nInstruction = 0x92;
        break;
    case CONDITION_LE:
        nInstruction = 0x9E;
        break;
    default:
        assert(0);
        break;
    }

	//setcc reg[l]
	m_pBlock->StreamWrite(3, 0x0F, nInstruction, 0xC0 | (0x00 << 3) | (m_nRegisterLookup[nRegister]));

	//movzx reg, reg[l]
	m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (m_nRegisterLookup[nRegister] << 3) | (m_nRegisterLookup[nRegister]));

	m_Shadow.Push(nRegister);
	m_Shadow.Push(REGISTER);
#endif
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

bool CCodeGen::IsTopContRelPair64()
{
    if(
        (m_Shadow.GetAt(0) == RELATIVE) &&
        (m_Shadow.GetAt(2) == RELATIVE) &&
        (m_Shadow.GetAt(4) == RELATIVE) &&
        (m_Shadow.GetAt(6) == RELATIVE)
        )
    {
        for(unsigned int i = 0; i < 2; i++)
        {
            uint32 nRelative[2];
            nRelative[0] = m_Shadow.GetAt((i * 4) + 1);
            nRelative[1] = m_Shadow.GetAt((i * 4) + 3);

            if((nRelative[0] - nRelative[1]) != 4)
            {
                return false;
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CCodeGen::IsTopContRelCstPair64()
{
    if(
        (m_Shadow.GetAt(0) == CONSTANT) &&
        (m_Shadow.GetAt(2) == CONSTANT) &&
        (m_Shadow.GetAt(4) == RELATIVE) &&
        (m_Shadow.GetAt(6) == RELATIVE)
        )
    {
        for(unsigned int i = 1; i < 2; i++)
        {
            uint32 nRelative[2];
            nRelative[0] = m_Shadow.GetAt((i * 4) + 1);
            nRelative[1] = m_Shadow.GetAt((i * 4) + 3);

            if((nRelative[0] - nRelative[1]) != 4)
            {
                return false;
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

void CCodeGen::StreamWriteByte(uint8 nByte)
{
    if(m_stream == NULL) return;
    m_stream->Write(&nByte, 1);
//    m_pBlock->StreamWriteByte(nByte);
}

void CCodeGen::StreamWriteAt(unsigned int position, uint8 value)
{
    if(m_stream == NULL) return;
    uint64 currentPosition = m_stream->Tell();
    m_stream->Seek(position, STREAM_SEEK_SET);
    m_stream->Write(&value, 1);
    m_stream->Seek(currentPosition, STREAM_SEEK_SET);
//    m_pBlock->StreamWriteAt(position, value);
}

size_t CCodeGen::StreamTell()
{
    if(m_stream == NULL) return 0;
//    return m_pBlock->StreamGetSize();
    return static_cast<size_t>(m_stream->Tell());
}

void CCodeGen::X86_RegImmOp(unsigned int nRegister, uint32 nConstant, unsigned int nOp)
{
    assert(nOp < 8);
    assert(nRegister < 8);

	if(GetMinimumConstantSize(nConstant) == 1)
	{
		//op reg, Immediate
		m_pBlock->StreamWrite(3, 0x83, 0xC0 | (nOp << 3) | (m_nRegisterLookup[nRegister]), (uint8)nConstant);
	}
	else
	{
		//op reg, Immediate
		m_pBlock->StreamWrite(2, 0x81, 0xC0 | (nOp << 3) | (m_nRegisterLookup[nRegister]));
		m_pBlock->StreamWriteWord(nConstant);
	}
}
