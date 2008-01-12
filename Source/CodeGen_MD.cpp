#include <assert.h>
#include "CodeGen.h"
#include "CodeGen_StackPatterns.h"

bool CCodeGen::Register128HasNextUse(XMMREGISTER registerId)
{
	unsigned int nCount = m_Shadow.GetCount();

	for(unsigned int i = 0; i < nCount; i += 2)
	{
		if(m_Shadow.GetAt(i) == REGISTER128)
		{
			if(m_Shadow.GetAt(i + 1) == registerId) return true;
		}
	}

	return false;
}

void CCodeGen::LoadRelative128InRegister(XMMREGISTER registerId, uint32 offset)
{
    m_Assembler.MovdquVo(registerId,
        CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, offset));
}

void CCodeGen::MD_PushRel(size_t offset)
{
    m_Shadow.Push(static_cast<uint32>(offset));
    m_Shadow.Push(RELATIVE128);
}

void CCodeGen::MD_PullRel(size_t offset)
{
    if(FitsPattern<SingleRegister128>())
    {
        XMMREGISTER valueRegister = static_cast<XMMREGISTER>(GetPattern<SingleRegister128>());
        m_Assembler.MovdquVo(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(offset)),
            valueRegister);
        FreeXmmRegister(valueRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::MD_PushReg(XMMREGISTER registerId)
{
    m_Shadow.Push(registerId);
    m_Shadow.Push(REGISTER128);
}

void CCodeGen::MD_And()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PandVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::MD_Or()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PorVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::MD_Not()
{
    if(FitsPattern<SingleRegister128>())
    {
        XMMREGISTER resultRegister(static_cast<XMMREGISTER>(GetPattern<SingleRegister128>()));
        assert(!Register128HasNextUse(resultRegister));
        XMMREGISTER tempRegister = AllocateXmmRegister();
        m_Assembler.PcmpeqdVo(tempRegister,
            CX86Assembler::MakeXmmRegisterAddress(tempRegister));
        m_Assembler.PxorVo(resultRegister,
            CX86Assembler::MakeXmmRegisterAddress(tempRegister));
        MD_PushReg(resultRegister);
    }
    else
    {
        assert(0);
    }
}

void CCodeGen::MD_SubB()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PsubbVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        assert(0);
    }
}
