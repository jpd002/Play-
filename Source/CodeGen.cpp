#include <assert.h>
#include "CodeGen.h"
#include "PtrMacro.h"
#include "CodeGen_StackPatterns.h"

using namespace boost;
using namespace Framework;
using namespace std;
using namespace std::tr1;

CX86Assembler::REGISTER CCodeGen::g_nBaseRegister = CX86Assembler::rBP;

CCodeGen::GenericOp Op_Add(&CX86Assembler::AddEd, &CX86Assembler::AddId);
CCodeGen::GenericOp Op_Adc(&CX86Assembler::AdcEd, &CX86Assembler::AdcId);
CCodeGen::GenericOp Op_And(&CX86Assembler::AndEd, &CX86Assembler::AndId);
CCodeGen::GenericOp Op_Cmp(&CX86Assembler::CmpEd, &CX86Assembler::CmpId);
CCodeGen::GenericOp Op_Sub(&CX86Assembler::SubEd, &CX86Assembler::SubId);
CCodeGen::GenericOp Op_Sbb(&CX86Assembler::SbbEd, &CX86Assembler::SbbId);

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

CCodeGen::CCodeGen() :
m_stream(NULL),
m_nBlockStarted(false),
m_Assembler(
        bind(&CCodeGen::StreamWriteByte, this, _1),
        bind(&CCodeGen::StreamWriteAt, this, _1, _2),
        bind(&CCodeGen::StreamTell, this)
        )
{

}

CCodeGen::~CCodeGen()
{
    
}

void CCodeGen::SetStream(CStream* stream)
{
    m_stream = stream;
}

void CCodeGen::Begin()
{
	assert(m_nBlockStarted == false);
	m_nBlockStarted = true;

	m_Shadow.Reset();

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
    else if(FitsPattern<SingleConstant>())
    {
        SingleConstant::PatternValue constant(GetPattern<SingleConstant>());
        //Skip the if block only if we have a "false" value
        CX86Assembler::LABEL ifLabel = m_Assembler.CreateLabel();
        if(!constant)
        {
            m_Assembler.JmpJb(ifLabel);
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
    assert(!RegisterHasNextUse(nRegister));
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
	else if(m_Shadow.GetAt(0) == REGISTER)
	{
		//That's fine
	}
	else
	{
		assert(0);
	}
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

        if(!RegisterHasNextUse(nRegister))
        {
		    FreeRegister(nRegister);
        }
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
        throw runtime_error("Not implemented");
/*
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
*/
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

void CCodeGen::Swap()
{
    uint32 value[2];
    uint32 valueType[2];

    for(int i = 1; i >= 0; i--)
    {
        valueType[i]   = m_Shadow.Pull();
        value[i]       = m_Shadow.Pull();
    }
    for(int i = 1; i >= 0; i--)
    {
        m_Shadow.Push(value[i]);
        m_Shadow.Push(valueType[i]);
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
	//else if((m_Shadow.GetAt(0) == REGISTER) && (m_Shadow.GetAt(2) == REGISTER))
	//{
	//	uint32 nRegister1, nRegister2;
	//
	//	m_Shadow.Pull();
	//	nRegister2 = m_Shadow.Pull();
	//	m_Shadow.Pull();
	//	nRegister1 = m_Shadow.Pull();

	//	//Can free this register
	//	if(!RegisterHasNextUse(nRegister2))
	//	{
	//		FreeRegister(nRegister2);
	//	}

	//	//add reg1, reg2
	//	m_pBlock->StreamWrite(2, 0x03, 0xC0 | (m_nRegisterLookup[nRegister1] << 3) | (m_nRegisterLookup[nRegister2]));

	//	m_Shadow.Push(nRegister1);
	//	m_Shadow.Push(REGISTER);
	//}
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
        uint32 value[4];
        uint32 valueType[4];

        for(int i = 3; i >= 0; i--)
        {
            valueType[i]   = m_Shadow.Pull();
            value[i]       = m_Shadow.Pull();
        }

		unsigned int resultLow = AllocateRegister();
		unsigned int resultHigh = AllocateRegister();
		
        EmitLoad(valueType[0], value[0], resultLow);
        EmitLoad(valueType[1], value[1], resultHigh);
		EmitOp(Op_Add, valueType[2], value[2], resultLow);
        EmitOp(Op_Adc, valueType[3], value[3], resultHigh);

		PushReg(resultLow);
		PushReg(resultHigh);
    }
}

void CCodeGen::And()
{
    if(FitsPattern<CommutativeRegisterConstant>())
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
    else if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());
        unsigned int resultRegister = AllocateRegister();
        LoadRelativeInRegister(resultRegister, ops.first);
        m_Assembler.AndEd(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        PushReg(resultRegister);
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
    if(FitsPattern<CommutativeRelativeConstant64>())
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
        uint32 value[4];
        uint32 valueType[4];

        for(int i = 3; i >= 0; i--)
        {
            valueType[i]   = m_Shadow.Pull();
            value[i]       = m_Shadow.Pull();
        }

		unsigned int resultLow = AllocateRegister();
		unsigned int resultHigh = AllocateRegister();
		
        EmitLoad(valueType[0], value[0], resultLow);
        EmitLoad(valueType[1], value[1], resultHigh);
		EmitOp(Op_And, valueType[2], value[2], resultLow);
        EmitOp(Op_And, valueType[3], value[3], resultHigh);

		PushReg(resultLow);
		PushReg(resultHigh);
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

	unsigned int additionalStack = 0;
#ifdef MACOSX
	//MacOS requires us to keep esp aligned on 16 bytes boundaries...
	additionalStack = (4 - (nParamCount & 3)) & 3;
	if(additionalStack != 0)
	{
		m_Assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP),
			additionalStack * 4);
	}
#endif

	for(unsigned int i = 0; i < nParamCount; i++)
	{
		if(m_Shadow.GetAt(0) == RELATIVE)
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
            CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), (nParamCount + additionalStack) * 4);
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
        (nCondition == CONDITION_EQ || nCondition == CONDITION_NE) &&
        FitsPattern<CommutativeRelativeConstant>()
        )
    {
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());
        unsigned int resultRegister = AllocateRegister(REGISTER_HASLOW);

        //cmp [relative], $constant
        m_Assembler.CmpId(
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first),
            ops.second);

        if(nCondition == CONDITION_EQ)
        {
            //sete reg[l]
            m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        }
        else if(nCondition == CONDITION_NE)
        {
            //setne reg[l]
            m_Assembler.SetneEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        }
        else
        {
            throw exception();
        }

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
        if(!RegisterHasNextUse(valueRegister))
        {
            FreeRegister(valueRegister);
        }

		//test reg, reg
        m_Assembler.TestEd(m_nRegisterLookupEx[valueRegister],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[valueRegister]));
		
        unsigned int resultRegister = AllocateRegister(REGISTER_HASLOW);

        //sete reg[l]
        m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));

		//movzx reg, reg[l]
        m_Assembler.MovzxEb(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));
		
        PushReg(resultRegister);
	}
    else if(FitsPattern<RegisterRegister>() && (nCondition == CONDITION_EQ))
    {
        RegisterRegister::PatternValue ops = GetPattern<RegisterRegister>();
        unsigned int resultRegister = AllocateRegister(REGISTER_HASLOW);

        //cmp op1, op2
        m_Assembler.CmpEd(m_nRegisterLookupEx[ops.first],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[ops.second]));

        //sete res[l]
        m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));

        if(!RegisterHasNextUse(ops.first)) FreeRegister(ops.first);
        if(!RegisterHasNextUse(ops.second)) FreeRegister(ops.second);

        PushReg(resultRegister);
    }
/*
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
        throw exception();
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

void CCodeGen::Div_Base(const MultFunction& function, bool isSigned)
{
    if(FitsPattern<RelativeConstant>())
    {
        RelativeConstant::PatternValue ops(GetPattern<RelativeConstant>());

        //We need eax and edx for this
        assert(!m_nRegisterAllocated[REGISTER_EAX] && !m_nRegisterAllocated[REGISTER_EDX]);
        m_nRegisterAllocated[REGISTER_EAX] = true;
        m_nRegisterAllocated[REGISTER_EDX] = true;
        unsigned int lowRegister = REGISTER_EAX;
        unsigned int highRegister = REGISTER_EDX;
        unsigned int tempRegister = AllocateRegister();

        LoadRelativeInRegister(lowRegister, ops.first);
        LoadConstantInRegister(tempRegister, ops.second);
        if(isSigned)
        {
            m_Assembler.Cdq();
        }
        else
        {
            m_Assembler.XorEd(m_nRegisterLookupEx[highRegister],
                CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[highRegister]));
        }
        function(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[tempRegister]));

        FreeRegister(tempRegister);

        PushReg(highRegister);
        PushReg(lowRegister);
    }
    else if(FitsPattern<ConstantRelative>())
    {
        ConstantRelative::PatternValue ops(GetPattern<ConstantRelative>());

        //We need eax and edx for this
        assert(!m_nRegisterAllocated[REGISTER_EAX] && !m_nRegisterAllocated[REGISTER_EDX]);
        m_nRegisterAllocated[REGISTER_EAX] = true;
        m_nRegisterAllocated[REGISTER_EDX] = true;
        unsigned int lowRegister = REGISTER_EAX;
        unsigned int highRegister = REGISTER_EDX;

        LoadConstantInRegister(lowRegister, ops.first);
        if(isSigned)
        {
            m_Assembler.Cdq();
        }
        else
        {
            m_Assembler.XorEd(m_nRegisterLookupEx[highRegister],
                CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[highRegister]));
        }
        function(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));

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
        if(isSigned)
        {
            m_Assembler.Cdq();
        }
        else
        {
            m_Assembler.XorEd(m_nRegisterLookupEx[highRegister],
                CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[highRegister]));
        }
        function(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));

        PushReg(highRegister);
        PushReg(lowRegister);
    }
    else if(FitsPattern<ConstantConstant>())
    {
        ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
        uint32 result = 0, remainder = 0;
        if(isSigned)
        {
            result = static_cast<int32>(ops.first) / static_cast<int32>(ops.second);
            remainder = static_cast<int32>(ops.first) % static_cast<int32>(ops.second);
        }
        else
        {
            result = ops.first / ops.second;
            remainder = ops.first % ops.second;
        }
        PushCst(remainder);
        PushCst(result);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::Div()
{
    Div_Base(bind(&CX86Assembler::DivEd, &m_Assembler, _1), false);
}

void CCodeGen::DivS()
{
    Div_Base(bind(&CX86Assembler::IdivEd, &m_Assembler, _1), true);
}

void CCodeGen::Lookup(uint32* table)
{
    if(FitsPattern<SingleRegister>())
    {
        SingleRegister::PatternValue indexRegister(GetPattern<SingleRegister>());
        unsigned int resultRegister = RegisterHasNextUse(indexRegister) ? AllocateRegister() : indexRegister;
        unsigned int baseRegister = AllocateRegister();
        m_Assembler.MovId(m_nRegisterLookupEx[baseRegister], 
            static_cast<uint32>(reinterpret_cast<uint8*>(table) - static_cast<uint8*>(NULL)));
        m_Assembler.MovEd(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeBaseIndexScaleAddress(m_nRegisterLookupEx[baseRegister], m_nRegisterLookupEx[resultRegister], 4));
        FreeRegister(baseRegister);
        PushReg(resultRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::Lzc()
{
    throw exception();
/*
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
*/
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
/*
	}
	else
	{
		assert(0);
	}
*/
}

void CCodeGen::Mult()
{
    Mult_Base(bind(&CX86Assembler::MulEd, &m_Assembler, _1), false);
}

void CCodeGen::MultS()
{
    Mult_Base(bind(&CX86Assembler::ImulEd, &m_Assembler, _1), true);
}

void CCodeGen::Mult_Base(const MultFunction& multFunction, bool isSigned)
{
    if(FitsPattern<CommutativeRelativeConstant>())
    {
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());

        //We need eax and edx for this
        if(m_nRegisterAllocated[REGISTER_EAX] || m_nRegisterAllocated[REGISTER_EDX])
        {
            throw runtime_error("Needed registers are allocated.");
        }
        m_nRegisterAllocated[REGISTER_EAX] = true;
        m_nRegisterAllocated[REGISTER_EDX] = true;
        unsigned int lowRegister = REGISTER_EAX;
        unsigned int highRegister = REGISTER_EDX;

        LoadRelativeInRegister(lowRegister, ops.first);
        LoadConstantInRegister(highRegister, ops.second);
        multFunction(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[highRegister]));

        PushReg(highRegister);
        PushReg(lowRegister);
    }
    else if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());

        //We need eax and edx for this
        if(m_nRegisterAllocated[REGISTER_EAX] || m_nRegisterAllocated[REGISTER_EDX])
        {
            throw runtime_error("Needed registers are allocated.");
        }
        m_nRegisterAllocated[REGISTER_EAX] = true;
        m_nRegisterAllocated[REGISTER_EDX] = true;
        unsigned int lowRegister = REGISTER_EAX;
        unsigned int highRegister = REGISTER_EDX;

        LoadRelativeInRegister(lowRegister, ops.first);
        multFunction(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));

        PushReg(highRegister);
        PushReg(lowRegister);
    }
    else if(FitsPattern<ConstantConstant>())
    {
        ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
        INTEGER64 result;
        if(isSigned)
        {
            result.q = static_cast<int32>(ops.first) * static_cast<int32>(ops.second);
        }
        else
        {
            result.q = static_cast<uint32>(ops.first) * static_cast<uint32>(ops.second);
        }
        PushCst(result.d1);
        PushCst(result.d0);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::Not()
{
    if(FitsPattern<SingleRegister>())
    {
        SingleRegister::PatternValue resultRegister = GetPattern<SingleRegister>();
        assert(!RegisterHasNextUse(resultRegister));
        m_Assembler.NotEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        PushReg(resultRegister);
    }
    else if(FitsPattern<SingleConstant>())
    {
        SingleConstant::PatternValue constant = GetPattern<SingleConstant>();
        PushCst(~constant);
    }
    else
    {
        throw runtime_error("Unhandled case.");
    }
}

void CCodeGen::Or()
{
	if(FitsPattern<RegisterRegister>())
	{
        RegisterRegister::PatternValue ops(GetPattern<RegisterRegister>());
		uint32 nRegister1 = ops.first;
        uint32 nRegister2 = ops.second;

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

        m_Assembler.OrEd(m_nRegisterLookupEx[nRegister1], 
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister2]));

        PushReg(nRegister1);
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

        if(ops.second != 0)
        {
		    PushCst(ops.second);
		    Or();
        }
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
    if(FitsPattern<SingleConstant>())
    {
        SingleConstant::PatternValue op = GetPattern<SingleConstant>();
        PushCst(static_cast<int16>(op));
    }
    else
    {
        ReduceToRegister();

        m_Shadow.Pull();
        uint32 nRegister = m_Shadow.Pull();

        m_Assembler.MovsxEw(m_nRegisterLookupEx[nRegister],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[nRegister]));

        PushReg(nRegister);
    }
}

void CCodeGen::Shl()
{
    if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops = GetPattern<RelativeRelative>();
        unsigned int shiftAmount = AllocateRegister(REGISTER_SHIFTAMOUNT);
        unsigned int resultRegister = AllocateRegister();
        LoadRelativeInRegister(resultRegister, ops.first);
        LoadRelativeInRegister(shiftAmount, ops.second);
        m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        FreeRegister(shiftAmount);
        PushReg(resultRegister);
    }
    else if(FitsPattern<ConstantRelative>())
    {
        ConstantRelative::PatternValue ops = GetPattern<ConstantRelative>();
        unsigned int shiftAmount = AllocateRegister(REGISTER_SHIFTAMOUNT);
        unsigned int resultRegister = AllocateRegister();
        LoadConstantInRegister(resultRegister, ops.first);
        LoadRelativeInRegister(shiftAmount, ops.second);
        m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        FreeRegister(shiftAmount);
        PushReg(resultRegister);
    }
    else if(FitsPattern<ConstantConstant>())
    {
        ConstantConstant::PatternValue ops = GetPattern<ConstantConstant>();
        PushCst(ops.first << ops.second);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::Shl(uint8 nAmount)
{
	if(FitsPattern<SingleRegister>())
	{
        SingleRegister::PatternValue op = GetPattern<SingleRegister>();
        unsigned int resultRegister;
        if(RegisterHasNextUse(op))
        {
            resultRegister = AllocateRegister();
            CopyRegister(resultRegister, op);
        }
        else
        {
            resultRegister = op;
        }

        m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]), nAmount);
        PushReg(resultRegister);
	}
	else if(FitsPattern<SingleRelative>())
	{
        UnaryRelativeSelfCallAsRegister(bind(&CCodeGen::Shl, this, nAmount));
	}
	else if(FitsPattern<SingleConstant>())
	{
        SingleConstant::PatternValue op = GetPattern<SingleConstant>();
		PushCst(op << nAmount);
	}
	else
	{
        throw exception();
	}
}

void CCodeGen::Shl64()
{
    {
        uint32 value[3];
        uint32 valueType[3];

        for(int i = 2; i >= 0; i--)
        {
            valueType[i]   = m_Shadow.Pull();
            value[i]       = m_Shadow.Pull();
        }

        assert(valueType[2] != CONSTANT);

        CX86Assembler::LABEL doneLabel = m_Assembler.CreateLabel();
        CX86Assembler::LABEL more32Label = m_Assembler.CreateLabel();

        unsigned int amountReg = AllocateRegister(REGISTER_SHIFTAMOUNT);
		unsigned int resultLow = AllocateRegister();
		unsigned int resultHigh = AllocateRegister();

        EmitLoad(valueType[0], value[0], resultLow);
        EmitLoad(valueType[1], value[1], resultHigh);
        EmitLoad(valueType[2], value[2], amountReg);

        m_Assembler.AndIb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[amountReg]),
            0x3F);
        m_Assembler.TestEb(m_nRegisterLookupEx[amountReg],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[amountReg]));
        m_Assembler.JeJb(doneLabel);
        m_Assembler.CmpIb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[amountReg]),
            0x20);
        m_Assembler.JaeJb(more32Label);

        m_Assembler.ShldEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultHigh]),
            m_nRegisterLookupEx[resultLow]);
        m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]));
        m_Assembler.JmpJb(doneLabel);

//$more32
        m_Assembler.MarkLabel(more32Label);

        m_Assembler.MovEd(m_nRegisterLookupEx[resultHigh],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]));
        m_Assembler.XorEd(m_nRegisterLookupEx[resultLow],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]));
        m_Assembler.AndIb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[amountReg]),
            0x1F);
        m_Assembler.ShlEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultHigh]));

//$done
        m_Assembler.MarkLabel(doneLabel);
        m_Assembler.ResolveLabelReferences();

		PushReg(resultLow);
		PushReg(resultHigh);

		FreeRegister(amountReg);
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
        throw runtime_error("Not implemented.");
/*
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
*/
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

void CCodeGen::Sra()
{
    if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops = GetPattern<RelativeRelative>();
        unsigned int shiftAmount = AllocateRegister(REGISTER_SHIFTAMOUNT);
        unsigned int resultRegister = AllocateRegister();
        LoadRelativeInRegister(resultRegister, ops.first);
        LoadRelativeInRegister(shiftAmount, ops.second);
        m_Assembler.SarEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        FreeRegister(shiftAmount);
        PushReg(resultRegister);
    }
    else if(FitsPattern<ConstantRelative>())
    {
        ConstantRelative::PatternValue ops = GetPattern<ConstantRelative>();
        unsigned int shiftAmount = AllocateRegister(REGISTER_SHIFTAMOUNT);
        unsigned int resultRegister = AllocateRegister();
        LoadConstantInRegister(resultRegister, ops.first);
        LoadRelativeInRegister(shiftAmount, ops.second);
        m_Assembler.SarEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        FreeRegister(shiftAmount);
        PushReg(resultRegister);
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
        UnaryRelativeSelfCallAsRegister(bind(&CCodeGen::Sra, this, nAmount));
	}
	else
	{
		assert(0);
	}
}

void CCodeGen::Sra64(uint8 nAmount)
{
	if(FitsPattern<SingleConstant64>())
	{
        assert(0);
	}
    else
    {
        uint32 value[2];
        uint32 valueType[2];

        for(int i = 1; i >= 0; i--)
        {
            valueType[i]   = m_Shadow.Pull();
            value[i]       = m_Shadow.Pull();
        }

		assert(nAmount < 0x40);

		if(nAmount >= 32)
		{
			uint32 resultLow = AllocateRegister();
			EmitLoad(valueType[1], value[1], resultLow);

            if(nAmount != 32)
            {
			    //sar reg, amount
			    m_Assembler.SarEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]),
				    nAmount & 0x1F);
            }

			PushReg(resultLow);
            SeX();
		}
		else //Amount < 32
		{
			unsigned int resultLow = AllocateRegister();
			unsigned int resultHigh = AllocateRegister();

            EmitLoad(valueType[0], value[0], resultLow);
            EmitLoad(valueType[1], value[1], resultHigh);

            if(nAmount != 0)
            {
			    //shrd nReg1, nReg2, nAmount
                m_Assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]),
                    m_nRegisterLookupEx[resultHigh],
                    nAmount);

			    //sar nReg2, nAmount
                m_Assembler.SarEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultHigh]), nAmount);
            }

			PushReg(resultLow);
			PushReg(resultHigh);
		}
    }
/*
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
*/
}

void CCodeGen::Srl()
{
    if(FitsPattern<RelativeRelative>())
    {
        RelativeRelative::PatternValue ops = GetPattern<RelativeRelative>();
        unsigned int shiftAmount = AllocateRegister(REGISTER_SHIFTAMOUNT);
        unsigned int resultRegister = AllocateRegister();
        LoadRelativeInRegister(resultRegister, ops.first);
        LoadRelativeInRegister(shiftAmount, ops.second);
        m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]));
        FreeRegister(shiftAmount);
        PushReg(resultRegister);
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
        UnaryRelativeSelfCallAsRegister(bind(&CCodeGen::Srl, this, nAmount));
	}
    else if(FitsPattern<SingleConstant>())
    {
        SingleConstant::PatternValue op = GetPattern<SingleConstant>();
        PushCst(op >> nAmount);
    }
	else
	{
		assert(0);
	}
}

void CCodeGen::Srl64()
{
    {
        uint32 value[3];
        uint32 valueType[3];

        for(int i = 2; i >= 0; i--)
        {
            valueType[i]   = m_Shadow.Pull();
            value[i]       = m_Shadow.Pull();
        }

        assert(valueType[2] != CONSTANT);

        CX86Assembler::LABEL doneLabel = m_Assembler.CreateLabel();
        CX86Assembler::LABEL more32Label = m_Assembler.CreateLabel();

        unsigned int amountReg = AllocateRegister(REGISTER_SHIFTAMOUNT);
		unsigned int resultLow = AllocateRegister();
		unsigned int resultHigh = AllocateRegister();

        EmitLoad(valueType[0], value[0], resultLow);
        EmitLoad(valueType[1], value[1], resultHigh);
        EmitLoad(valueType[2], value[2], amountReg);

        m_Assembler.AndIb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[amountReg]),
            0x3F);
        m_Assembler.TestEb(m_nRegisterLookupEx[amountReg],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[amountReg]));
        m_Assembler.JeJb(doneLabel);
        m_Assembler.CmpIb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[amountReg]),
            0x20);
        m_Assembler.JaeJb(more32Label);

        m_Assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]),
            m_nRegisterLookupEx[resultHigh]);
        m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultHigh]));
        m_Assembler.JmpJb(doneLabel);

//$more32
        m_Assembler.MarkLabel(more32Label);
        m_Assembler.MovEd(m_nRegisterLookupEx[resultLow],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultHigh]));
        m_Assembler.XorEd(m_nRegisterLookupEx[resultHigh],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultHigh]));
        m_Assembler.AndIb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[amountReg]),
            0x1F);
        m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]));

//$done
        m_Assembler.MarkLabel(doneLabel);
        m_Assembler.ResolveLabelReferences();

        FreeRegister(amountReg);
		PushReg(resultLow);
		PushReg(resultHigh);
    }
}

void CCodeGen::Srl64(uint8 nAmount)
{
	if(FitsPattern<SingleConstant64>())
	{
        SingleConstant64::PatternValue ops(GetPattern<SingleConstant64>());
        ops.q >>= nAmount;
        PushCst(ops.d0);
        PushCst(ops.d1);
	}
    else
    {
        uint32 value[2];
        uint32 valueType[2];

        for(int i = 1; i >= 0; i--)
        {
            valueType[i]   = m_Shadow.Pull();
            value[i]       = m_Shadow.Pull();
        }

		assert(nAmount < 0x40);

		if(nAmount >= 32)
		{
			uint32 resultLow = AllocateRegister();
			EmitLoad(valueType[1], value[1], resultLow);

            if(nAmount != 32)
            {
			    //shr reg, amount
			    m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]),
				    nAmount & 0x1F);
            }

			PushReg(resultLow);
			PushCst(0);
		}
		else //Amount < 32
		{
			unsigned int resultLow = AllocateRegister();
			unsigned int resultHigh = AllocateRegister();

            EmitLoad(valueType[0], value[0], resultLow);
            EmitLoad(valueType[1], value[1], resultHigh);

            if(nAmount != 0)
            {
			    //shrd nReg1, nReg2, nAmount
                m_Assembler.ShrdEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultLow]),
                    m_nRegisterLookupEx[resultHigh],
                    nAmount);

			    //shr nReg2, nAmount
                m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultHigh]), nAmount);
            }

			PushReg(resultLow);
			PushReg(resultHigh);
		}
    }
}

void CCodeGen::Sub()
{
    if((m_Shadow.GetAt(0) == RELATIVE) && (m_Shadow.GetAt(2) == RELATIVE))
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
    else if(FitsPattern<ConstantConstant>())
    {
        ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
        PushCst(ops.first - ops.second);
    }
	else
	{
        throw exception();
	}
}

void CCodeGen::Sub64()
{
    {
        uint32 value[4];
        uint32 valueType[4];

        for(int i = 3; i >= 0; i--)
        {
            valueType[i]   = m_Shadow.Pull();
            value[i]       = m_Shadow.Pull();
        }

		unsigned int resultLow = AllocateRegister();
		unsigned int resultHigh = AllocateRegister();
		
        EmitLoad(valueType[0], value[0], resultLow);
        EmitLoad(valueType[1], value[1], resultHigh);
		EmitOp(Op_Sub, valueType[2], value[2], resultLow);
        EmitOp(Op_Sbb, valueType[3], value[3], resultHigh);

		PushReg(resultLow);
		PushReg(resultHigh);
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
    else if(FitsPattern<CommutativeRegisterConstant>())
    {
        CommutativeRegisterConstant::PatternValue ops(GetPattern<CommutativeRegisterConstant>());
        unsigned int resultRegister = ops.first;
        assert(!RegisterHasNextUse(resultRegister));
        if(ops.second != 0)
        {
            m_Assembler.XorId(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[resultRegister]),
                ops.second);
        }
        PushReg(resultRegister);
    }
    else if(FitsPattern<ConstantConstant>())
    {
        ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
        PushCst(ops.first ^ ops.second);
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
        uint32 value[4];
        uint32 valueType[4];

        for(int i = 3; i >= 0; i--)
        {
            valueType[i]   = m_Shadow.Pull();
            value[i]       = m_Shadow.Pull();
        }

        unsigned int resultRegister = AllocateRegister(REGISTER_HASLOW);
        unsigned int tempRegister = AllocateRegister(REGISTER_HASLOW);

        EmitLoad(valueType[0], value[0], resultRegister);
        EmitOp(Op_Cmp, valueType[2], value[2], resultRegister);
        m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));

        EmitLoad(valueType[1], value[1], tempRegister);
        EmitOp(Op_Cmp, valueType[3], value[3], tempRegister);
        m_Assembler.SeteEb(CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[tempRegister]));

        m_Assembler.AndEd(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[tempRegister]));
        m_Assembler.MovzxEb(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeByteRegisterAddress(m_nRegisterLookupEx[resultRegister]));

        FreeRegister(tempRegister);
        PushReg(resultRegister);
	}
}

void CCodeGen::Cmp64Lt(bool nSigned, bool nOrEqual)
{
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
        EmitLoad(nValueType[1], nValue[1], nRegister);
        EmitOp(Op_Cmp, nValueType[3], nValue[3], nRegister);

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

        EmitLoad(nValueType[0], nValue[0], nRegister);
        EmitOp(Op_Cmp, nValueType[2], nValue[2], nRegister);

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
}

void CCodeGen::StreamWriteAt(unsigned int position, uint8 value)
{
    if(m_stream == NULL) return;
    uint64 currentPosition = m_stream->Tell();
    m_stream->Seek(position, Framework::STREAM_SEEK_SET);
    m_stream->Write(&value, 1);
    m_stream->Seek(currentPosition, Framework::STREAM_SEEK_SET);
}

size_t CCodeGen::StreamTell()
{
    if(m_stream == NULL) return 0;
    return static_cast<size_t>(m_stream->Tell());
}

void CCodeGen::EmitLoad(uint32 valueType, uint32 value, unsigned int registerId)
{
    switch(valueType)
    {
    case CONSTANT:
        LoadConstantInRegister(registerId, value);
        break;
    case RELATIVE:
        LoadRelativeInRegister(registerId, value);
        break;
    case REGISTER:
        assert(!RegisterHasNextUse(value));
        CopyRegister(registerId, value);
        FreeRegister(value);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen::EmitOp(const GenericOp& operation, uint32 valueType, uint32 value, unsigned int registerId)
{
    switch(valueType)
    {
    case CONSTANT:
        ((m_Assembler).*(operation.constantOp))(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[registerId]),
            value);
        break;
    case RELATIVE:
        ((m_Assembler).*(operation.relativeOp))(m_nRegisterLookupEx[registerId],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, value));
        break;
    default:
        assert(0);
        break;
    }
}
