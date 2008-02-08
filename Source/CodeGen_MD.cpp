#include "CodeGen.h"
#include "CodeGen_StackPatterns.h"

using namespace std;

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
        throw exception();
    }
}

void CCodeGen::MD_PushReg(XMMREGISTER registerId)
{
    m_Shadow.Push(registerId);
    m_Shadow.Push(REGISTER128);
}

void CCodeGen::MD_AddH()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PaddwVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_AddWUS()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        unsigned int tempRegister = AllocateRegister();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        for(int i = 3; i >= 0; i--)
        {
            CX86Assembler::LABEL overflowLabel = m_Assembler.CreateLabel();
            CX86Assembler::LABEL doneLabel = m_Assembler.CreateLabel();
            m_Assembler.MovEd(m_nRegisterLookupEx[tempRegister],
                CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first + (i * 4)));
            m_Assembler.AddEd(m_nRegisterLookupEx[tempRegister],
                CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second + (i * 4)));
            m_Assembler.JcJb(overflowLabel);
            m_Assembler.PushEd(CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[tempRegister]));
            m_Assembler.JmpJb(doneLabel);
            m_Assembler.MarkLabel(overflowLabel);
            m_Assembler.PushId(0xFFFFFFFF);
            m_Assembler.MarkLabel(doneLabel);
        }
        FreeRegister(tempRegister);
        m_Assembler.ResolveLabelReferences();
        m_Assembler.MovdquVo(resultRegister,
            CX86Assembler::MakeIndRegAddress(CX86Assembler::rSP));
        m_Assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP),
            0x10);
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
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
        throw exception();
    }
}

void CCodeGen::MD_CmpGtH()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PcmpgtwVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_MaxH()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PmaxswVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_MinH()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PminswVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
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
        throw exception();
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
        throw exception();
    }
}

void CCodeGen::MD_PackHB()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        XMMREGISTER tempRegister = AllocateXmmRegister();
        XMMREGISTER maskRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.second);
        LoadRelative128InRegister(tempRegister, ops.first);

        //Generate mask (0x00FF x8)
        m_Assembler.PcmpeqdVo(maskRegister,
            CX86Assembler::MakeXmmRegisterAddress(maskRegister));
        m_Assembler.PsrlwVo(maskRegister, 0x08);
        
        //Mask both operands
        m_Assembler.PandVo(resultRegister,
            CX86Assembler::MakeXmmRegisterAddress(maskRegister));
        m_Assembler.PandVo(tempRegister,
            CX86Assembler::MakeXmmRegisterAddress(maskRegister));

        //Pack
        m_Assembler.Packuswb(resultRegister,
            CX86Assembler::MakeXmmRegisterAddress(tempRegister));

        FreeXmmRegister(maskRegister);
        FreeXmmRegister(tempRegister);

        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_SrlH(uint8 amount)
{
    if(FitsPattern<SingleRelative128>())
    {
        SingleRelative128::PatternValue op(GetPattern<SingleRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, op);
        m_Assembler.PsrlwVo(resultRegister, amount);
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_Srl256()
{
    if(m_Shadow.Pull() != RELATIVE) assert(0);
    uint32 shiftAmount = m_Shadow.Pull();
    if(m_Shadow.Pull() != RELATIVE128) assert(0);
    uint32 operand2 = m_Shadow.Pull();
    if(m_Shadow.Pull() != RELATIVE128) assert(0);
    uint32 operand1 = m_Shadow.Pull();

    assert(m_nRegisterAllocated[1] == false);       //eCX
    assert(m_nRegisterAllocated[4] == false);       //eSI
    assert(m_nRegisterAllocated[5] == false);       //eDI

    XMMREGISTER resultRegister = AllocateXmmRegister();

	//Copy both registers on the stack
	//-----------------------------
    m_Assembler.SubId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), 0x30);
    m_Assembler.MovGd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rDI), CX86Assembler::rSP);
    m_Assembler.LeaGd(CX86Assembler::rSI, CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, operand2));
    m_Assembler.MovId(CX86Assembler::rCX, 0x10);
    m_Assembler.RepMovsb();
    m_Assembler.LeaGd(CX86Assembler::rSI, CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, operand1));
    m_Assembler.MovId(CX86Assembler::rCX, 0x10);
    m_Assembler.RepMovsb();

	//Setup SA
	//-----------------------------
    m_Assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, shiftAmount));
    m_Assembler.AndId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), 0x7F);
    m_Assembler.ShrEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), 0x03);

	//Setup ESI
	//-----------------------------
    m_Assembler.LeaGd(CX86Assembler::rSI, 
        CX86Assembler::MakeBaseIndexScaleAddress(CX86Assembler::rSP, CX86Assembler::rAX, 1));

	//Setup ECX
	//-----------------------------
    m_Assembler.MovId(CX86Assembler::rCX, 0x10);

	//Generate result
	//-----------------------------
    m_Assembler.RepMovsb();

	//Load result and free Stack
	//-----------------------------
    m_Assembler.MovdquVo(resultRegister, 
        CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, 0x20));
    m_Assembler.AddId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rSP), 0x30);

    MD_PushReg(resultRegister);
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
        throw exception();
    }
}

void CCodeGen::MD_SubW()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PsubdVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_UnpackLowerBH()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.second);
        m_Assembler.PunpcklbwVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_UnpackUpperBH()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.second);
        m_Assembler.PunpckhbwVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_Xor()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        m_Assembler.PxorVo(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}
