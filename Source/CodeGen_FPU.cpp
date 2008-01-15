#include <assert.h>
#include "CodeGen_FPU.h"
#include "CodeGen_StackPatterns.h"
#include "PtrMacro.h"

using namespace CodeGen;

CCacheBlock*	CFPU::m_pBlock = NULL;

void CFPU::Begin(CCacheBlock* pBlock)
{
	m_pBlock = pBlock;
}

void CFPU::End()
{

}

void CFPU::PushSingle(void* pAddr)
{
	//fld dword ptr[pAddr]
	m_pBlock->StreamWrite(2, 0xD9, 0x05);
	m_pBlock->StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
}

void CFPU::PushSingleImm(float nValue)
{
	if(nValue == 0.0)
	{
		//fldz
		m_pBlock->StreamWrite(2, 0xD9, 0xEE);
	}
	else
	{
		//push nValue
		m_pBlock->StreamWrite(1, 0x68);
		m_pBlock->StreamWriteWord(*(uint32*)&nValue);

		//fld dword ptr [esp]
		m_pBlock->StreamWrite(3, 0xD9, 0x04, 0x24);

		//add esp, 4
		m_pBlock->StreamWrite(3, 0x83, 0xC4, 0x04);
	}
}

//void CFPU::PushWord(void* pAddr)
//{
//	//fild dword ptr[pAddr]
//	m_pBlock->StreamWrite(2, 0xDB, 0x00 | (0x00 << 3) | 0x05);
//	m_pBlock->StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
//}

void CFPU::PullSingle(void* pAddr)
{
	//fstp dword ptr[pAddr]
	m_pBlock->StreamWrite(2, 0xD9, 0x00 | (0x03 << 3) | 0x05);
	m_pBlock->StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
}

//void CFPU::PullWord(void* pAddr)
//{
//	//fistp dword ptr[pAddr]
//	m_pBlock->StreamWrite(2, 0xDB, 0x00 | (0x03 << 3) | 0x05);
//	m_pBlock->StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
//}


void CFPU::PushRoundingMode()
{
	//sub esp, 4
	m_pBlock->StreamWrite(3, 0x83, 0xEC, 0x04);

	//fwait
	m_pBlock->StreamWrite(1, 0x9B);

	//fnstcw word ptr [esp]
	m_pBlock->StreamWrite(3, 0xD9, 0x3C, 0x24);
}


void CFPU::PullRoundingMode()
{
	//fldcw word ptr [esp]
	m_pBlock->StreamWrite(3, 0xD9, 0x2C, 0x24);

	//add esp, 4
	m_pBlock->StreamWrite(3, 0x83, 0xC4, 0x04);
}

void CFPU::SetRoundingMode(ROUNDMODE nMode)
{
	//sub esp, 4
	m_pBlock->StreamWrite(3, 0x83, 0xEC, 0x04);

	//fwait
	m_pBlock->StreamWrite(1, 0x9B);

	//fnstcw word ptr [esp]
	m_pBlock->StreamWrite(3, 0xD9, 0x3C, 0x24);

	//pop eax
	m_pBlock->StreamWrite(1, 0x58);

	//and eax, 0xFFFFF3FF
	m_pBlock->StreamWrite(1, 0x25);
	m_pBlock->StreamWriteWord(0xFFFFF3FF);

	//or eax, nMode << 10
	m_pBlock->StreamWrite(1, 0x0D);
	m_pBlock->StreamWriteWord(nMode << 10);

	//push eax
	m_pBlock->StreamWrite(1, 0x50);

	//fldcw word ptr [esp]
	m_pBlock->StreamWrite(3, 0xD9, 0x2C, 0x24);

	//add esp, 4
	m_pBlock->StreamWrite(3, 0x83, 0xC4, 0x04);
}

void CFPU::Add()
{
	//faddp
	m_pBlock->StreamWrite(2, 0xDE, 0xC1);
}

void CFPU::Abs()
{
	//fabs
	m_pBlock->StreamWrite(2, 0xD9, 0xE1);
}

void CFPU::Dup()
{
	//fld st
	m_pBlock->StreamWrite(2, 0xD9, 0xC0);
}

void CFPU::Sub()
{
	//fsubp
	m_pBlock->StreamWrite(2, 0xDE, 0xE9);
}

void CFPU::Mul()
{
	//fmulp
	m_pBlock->StreamWrite(2, 0xDE, 0xC9);
}

void CFPU::Div()
{
	//fdivp
	m_pBlock->StreamWrite(2, 0xDE, 0xF9);
}

void CFPU::Neg()
{
	//fchs
	m_pBlock->StreamWrite(2, 0xD9, 0xE0);
}

void CFPU::Rcpl()
{
	//fld1
	m_pBlock->StreamWrite(2, 0xD9, 0xE8);

	//fdivrp
	m_pBlock->StreamWrite(2, 0xDE, 0xF1);
}

void CFPU::Sqrt()
{
	//fsqrt
	m_pBlock->StreamWrite(2, 0xD9, 0xFA);
}

void CFPU::Cmp(CCodeGen::CONDITION nCondition)
{
	unsigned int nRegister;

	//fcomip
	m_pBlock->StreamWrite(2, 0xDF, 0xF1);

	//fstp
	m_pBlock->StreamWrite(2, 0xDD, 0xD8);

	nRegister = CCodeGen::AllocateRegister(CCodeGen::REGISTER_HASLOW);

	switch(nCondition)
	{
	case CCodeGen::CONDITION_EQ:
		//sete reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister]));
		break;
	case CCodeGen::CONDITION_BL:
		//setb reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x92, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister]));
		break;
	case CCodeGen::CONDITION_BE:
		//setbe reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x96, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister]));
		break;
	case CCodeGen::CONDITION_AB:
		//seta reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x97, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister]));
		break;
	default:
		assert(0);
		break;
	}

	//movzx reg, reg[l]
	m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister] << 3) | (CCodeGen::m_nRegisterLookup[nRegister]));

	CCodeGen::PushReg(nRegister);
}

void CFPU::Round()
{
	//frndint
	m_pBlock->StreamWrite(2, 0xD9, 0xFC);
}

//New stuff

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
    else
    {
        assert(0);
    }
}

void CCodeGen::FP_Sub()
{
    if(FitsPattern<DualFpSingleRelative>())
    {
        DualFpSingleRelative::PatternValue ops = GetPattern<DualFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        FP_LoadSingleRelativeInRegister(resultRegister, ops.first);
        m_Assembler.SubssEd(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::FP_Mul()
{
    if(FitsPattern<DualFpSingleRelative>())
    {
        DualFpSingleRelative::PatternValue ops = GetPattern<DualFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        FP_LoadSingleRelativeInRegister(resultRegister, ops.first);
        m_Assembler.MulssEd(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::FP_Div()
{
    if(FitsPattern<DualFpSingleRelative>())
    {
        DualFpSingleRelative::PatternValue ops = GetPattern<DualFpSingleRelative>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        FP_LoadSingleRelativeInRegister(resultRegister, ops.first);
        m_Assembler.DivssEd(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        FP_PushSingleReg(resultRegister);
    }
    else
    {
        assert(0);
    }
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
        assert(0);
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
        assert(0);
    }
}
