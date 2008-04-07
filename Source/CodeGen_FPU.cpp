#include "CodeGen.h"
#include "CodeGen_StackPatterns.h"

using namespace CodeGen;
using namespace std;

bool CCodeGen::RegisterFpSingleHasNextUse(XMMREGISTER registerId)
{
	unsigned int nCount = m_Shadow.GetCount();

	for(unsigned int i = 0; i < nCount; i += 2)
	{
		if(m_Shadow.GetAt(i) == FP_SINGLE_REGISTER)
		{
			if(m_Shadow.GetAt(i + 1) == registerId) return true;
		}
	}

	return false;
}

void CCodeGen::FP_PushSingleReg(XMMREGISTER registerId)
{
    m_Shadow.Push(registerId);
    m_Shadow.Push(FP_SINGLE_REGISTER);
}

void CCodeGen::FP_PushSingle(size_t offset)
{
    m_Shadow.Push(static_cast<uint32>(offset));
    m_Shadow.Push(FP_SINGLE_RELATIVE);
}

void CCodeGen::FP_LoadSingleRelativeInRegister(XMMREGISTER destination, uint32 source)
{
    m_Assembler.MovssEd(destination,
        CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, source));
}

void CCodeGen::FP_PushWord(size_t offset)
{
    XMMREGISTER resultRegister = AllocateXmmRegister();
    m_Assembler.Cvtsi2ssEd(resultRegister,
        CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(offset)));
    FP_PushSingleReg(resultRegister);
}

void CCodeGen::FP_PullSingle(size_t offset)
{
    if(FitsPattern<SingleFpSingleRegister>())
    {
        XMMREGISTER valueRegister = static_cast<XMMREGISTER>(GetPattern<SingleFpSingleRegister>());
        m_Assembler.MovssEd(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(offset)),
            valueRegister);
        FreeXmmRegister(valueRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::FP_PullWordTruncate(size_t offset)
{
    if(FitsPattern<SingleFpSingleRelative>())
    {
        SingleFpSingleRelative::PatternValue op = GetPattern<SingleFpSingleRelative>();
        unsigned int valueRegister = AllocateRegister();
        m_Assembler.Cvttss2siEd(m_nRegisterLookupEx[valueRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, op));
        m_Assembler.MovGd(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(offset)),
            m_nRegisterLookupEx[valueRegister]);
        FreeRegister(valueRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::FP_GenericTwoOperand(const MdTwoOperandFunction& instruction)
{
    if(FitsPattern<DualFpSingleRelative>())
    {
        DualFpSingleRelative::PatternValue ops = GetPattern<DualFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        FP_LoadSingleRelativeInRegister(resultRegister, ops.first);
        instruction(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        FP_PushSingleReg(resultRegister);
    }
    else if(FitsPattern<FpSingleRelativeRegister>())
    {
        FpSingleRelativeRegister::PatternValue ops = GetPattern<FpSingleRelativeRegister>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        {
            XMMREGISTER sourceRegister = static_cast<XMMREGISTER>(ops.second);
            FP_LoadSingleRelativeInRegister(resultRegister, ops.first);
            instruction(resultRegister,
                CX86Assembler::MakeXmmRegisterAddress(sourceRegister));
            if(!RegisterFpSingleHasNextUse(sourceRegister))
            {
                FreeXmmRegister(sourceRegister);
            }
        }
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::FP_Add()
{
    if(FitsPattern<DualFpSingleRelative>())
    {
        DualFpSingleRelative::PatternValue ops = GetPattern<DualFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        FP_LoadSingleRelativeInRegister(resultRegister, ops.first);
        m_Assembler.AddssEd(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        FP_PushSingleReg(resultRegister);
    }
    else if(FitsPattern<CommutativeFpSingleRegisterRelative>())
    {
        CommutativeFpSingleRegisterRelative::PatternValue ops = GetPattern<CommutativeFpSingleRegisterRelative>();
        XMMREGISTER destinationRegister = static_cast<XMMREGISTER>(ops.first);
        XMMREGISTER resultRegister = RegisterFpSingleHasNextUse(destinationRegister) ? AllocateXmmRegister() : destinationRegister;
        m_Assembler.AddssEd(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::FP_Abs()
{
    if(FitsPattern<SingleFpSingleRelative>())
    {
        SingleFpSingleRelative::PatternValue op = GetPattern<SingleFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        unsigned int tempRegister = AllocateRegister();
        m_Assembler.MovId(m_nRegisterLookupEx[tempRegister], 0x7FFFFFFF);
        m_Assembler.AndEd(m_nRegisterLookupEx[tempRegister],
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, op));
        m_Assembler.MovdVo(resultRegister,
            CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[tempRegister]));
        FreeRegister(tempRegister);
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        throw runtime_error("Not implemented.");
    }
}

void CCodeGen::FP_Sub()
{
    FP_GenericTwoOperand(bind(&CX86Assembler::SubssEd, m_Assembler, _1, _2));
}

void CCodeGen::FP_Mul()
{
    FP_GenericTwoOperand(bind(&CX86Assembler::MulssEd, m_Assembler, _1, _2));
}

void CCodeGen::FP_Div()
{
    FP_GenericTwoOperand(bind(&CX86Assembler::DivssEd, m_Assembler, _1, _2));
}

void CCodeGen::FP_Cmp(CCodeGen::CONDITION condition)
{
    if(FitsPattern<DualFpSingleRelative>())
    {
        DualFpSingleRelative::PatternValue ops = GetPattern<DualFpSingleRelative>();
        CX86Assembler::SSE_CMP_TYPE conditionCode;
        switch(condition)
        {
        case CONDITION_EQ:
            conditionCode = CX86Assembler::SSE_CMP_EQ;
            break;
        case CONDITION_BL:
            conditionCode = CX86Assembler::SSE_CMP_LT;
            break;
        case CONDITION_BE:
            conditionCode = CX86Assembler::SSE_CMP_LE;
            break;
        default:
            assert(0);
            break;
        }
        XMMREGISTER tempResultRegister = AllocateXmmRegister();
        unsigned int resultRegister = AllocateRegister();
        FP_LoadSingleRelativeInRegister(tempResultRegister, ops.second);
        m_Assembler.CmpssEd(tempResultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first),
            conditionCode);
        //Can't move directly to register using MOVSS, so we use CVTTSS2SI
        //0x00000000 -- CVT -> zero
        //0xFFFFFFFF -- CVT -> not zero
        m_Assembler.Cvttss2siEd(m_nRegisterLookupEx[resultRegister],
            CX86Assembler::MakeXmmRegisterAddress(tempResultRegister));
        FreeXmmRegister(tempResultRegister);
        PushReg(resultRegister);
    }
    else
    {
        throw runtime_error("Not implemented.");
    }
}

void CCodeGen::FP_Neg()
{
    if(FitsPattern<SingleFpSingleRelative>())
    {
        SingleFpSingleRelative::PatternValue op = GetPattern<SingleFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        m_Assembler.PxorVo(resultRegister, 
            CX86Assembler::MakeXmmRegisterAddress(resultRegister));
        m_Assembler.SubssEd(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, op));
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        throw runtime_error("Not implemented.");
    }
}

void CCodeGen::FP_Sqrt()
{
    if(FitsPattern<SingleFpSingleRelative>())
    {
        SingleFpSingleRelative::PatternValue op = GetPattern<SingleFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        m_Assembler.SqrtssEd(resultRegister, 
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, op));
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        throw runtime_error("Not implemented.");
    }
}

void CCodeGen::FP_Rsqrt()
{
    if(FitsPattern<SingleFpSingleRelative>())
    {
        SingleFpSingleRelative::PatternValue op = GetPattern<SingleFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        m_Assembler.RsqrtssEd(resultRegister, 
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, op));
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        throw runtime_error("Not implemented.");
    }
}
