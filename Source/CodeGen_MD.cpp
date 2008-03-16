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

void CCodeGen::CopyRegister128(XMMREGISTER destination, XMMREGISTER source)
{
    m_Assembler.MovapsVo(CX86Assembler::MakeXmmRegisterAddress(destination),
        source);
}

void CCodeGen::LoadRelative128InRegister(XMMREGISTER registerId, uint32 offset)
{
    m_Assembler.MovapsVo(registerId,
        CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, offset));
}

void CCodeGen::MD_PushRel(size_t offset)
{
    m_Shadow.Push(static_cast<uint32>(offset));
    m_Shadow.Push(RELATIVE128);
}

void CCodeGen::MD_PushRelExpand(size_t offset)
{
    //Need to convert to a register

    XMMREGISTER valueRegister = AllocateXmmRegister();
    m_Assembler.MovssEd(valueRegister,
        CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(offset)));
    m_Assembler.ShufpsVo(valueRegister,
        CX86Assembler::MakeXmmRegisterAddress(valueRegister), 0x00);

    MD_PushReg(valueRegister);
}

void CCodeGen::MD_PushCstExpand(float constant)
{
    XMMREGISTER valueRegister = AllocateXmmRegister();
    unsigned int tempRegister = AllocateRegister();
    m_Assembler.MovId(m_nRegisterLookupEx[tempRegister],
        *reinterpret_cast<uint32*>(&constant));
    m_Assembler.MovdVo(valueRegister,
        CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[tempRegister]));
    m_Assembler.ShufpsVo(valueRegister,
        CX86Assembler::MakeXmmRegisterAddress(valueRegister), 0x00);
    MD_PushReg(valueRegister);
    FreeRegister(tempRegister);
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

void CCodeGen::MD_PullRel(size_t offset0, size_t offset1, size_t offset2, size_t offset3)
{
    if(FitsPattern<SingleRegister128>())
    {
        XMMREGISTER valueRegister = static_cast<XMMREGISTER>(GetPattern<SingleRegister128>());

	    if(
            offset0 != SIZE_MAX && 
            offset1 != SIZE_MAX &&
            offset2 != SIZE_MAX &&
            offset3 != SIZE_MAX
            )
	    {
		    //All elements are non-null
		    if((offset1 == offset0 + 4) && (offset2 == offset1 + 4) && (offset3 == offset2 + 4))
		    {
                m_Assembler.MovapsVo(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(offset0)),
                    valueRegister);
		    }
	    }
        else
        {
	        size_t offset[4];
	        uint8 shuffle[4] = { 0x00, 0xE5, 0xEA, 0xFF };

	        offset[0] = offset0;
	        offset[1] = offset1;
	        offset[2] = offset2;
	        offset[3] = offset3;

            for(unsigned int i = 0; i < 4; i++)
	        {
		        if(offset[i] != SIZE_MAX)
		        {
			        if(i != 0)
			        {
                        m_Assembler.ShufpsVo(valueRegister,
                            CX86Assembler::MakeXmmRegisterAddress(valueRegister), shuffle[i]);
			        }

                    m_Assembler.MovssEd(CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, static_cast<uint32>(offset[i])),
                        valueRegister);
		        }
	        }
        }

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

void CCodeGen::MD_GenericPackedShift(const PackedShiftFunction& instruction, uint8 amount)
{
    if(FitsPattern<SingleRelative128>())
    {
        SingleRelative128::PatternValue op(GetPattern<SingleRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, op);
        instruction(resultRegister, amount);
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_GenericOneOperand(const MdTwoOperandFunction& instruction)
{
    if(FitsPattern<SingleRelative128>())
    {
        SingleRelative128::PatternValue op = GetPattern<SingleRelative128>();
        XMMREGISTER resultRegister = AllocateXmmRegister();
        instruction(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, op));
        MD_PushReg(resultRegister);
    }
    else if(FitsPattern<SingleRegister128>())
    {
        XMMREGISTER valueRegister = static_cast<XMMREGISTER>(GetPattern<SingleRegister128>());
        assert(!Register128HasNextUse(valueRegister));
        instruction(valueRegister,
            CX86Assembler::MakeXmmRegisterAddress(valueRegister));
        MD_PushReg(valueRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_GenericTwoOperand(const MdTwoOperandFunction& instruction)
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.first);
        instruction(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second));
        MD_PushReg(resultRegister);
    }
    else if(FitsPattern<RelativeRegister128>())
    {
        RelativeRegister128::PatternValue ops(GetPattern<RelativeRegister128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        {
            XMMREGISTER valueRegister = static_cast<XMMREGISTER>(ops.second);
            LoadRelative128InRegister(resultRegister, ops.first);
            instruction(resultRegister,
                CX86Assembler::MakeXmmRegisterAddress(valueRegister));
            if(!Register128HasNextUse(valueRegister))
            {
                FreeXmmRegister(valueRegister);
            }
        }
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_GenericTwoOperandReversed(const MdTwoOperandFunction& instruction)
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.second);
        instruction(resultRegister,
            CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first));
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_AddH()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::PaddwVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_AddWSS()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();

        {
            unsigned int tempRegister = AllocateRegister();
            unsigned int positiveValueRegister = AllocateRegister();
            unsigned int negativeValueRegister = AllocateRegister();

            //Prepare registers
            m_Assembler.MovId(m_nRegisterLookupEx[positiveValueRegister],
                0x7FFFFFFF);
            m_Assembler.MovId(m_nRegisterLookupEx[negativeValueRegister],
                0x80000000);
            for(int i = 3; i >= 0; i--)
            {
                CX86Assembler::LABEL doneLabel = m_Assembler.CreateLabel();
                m_Assembler.MovEd(m_nRegisterLookupEx[tempRegister],
                    CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.first + (i * 4)));
                m_Assembler.AddEd(m_nRegisterLookupEx[tempRegister],
                    CX86Assembler::MakeIndRegOffAddress(g_nBaseRegister, ops.second + (i * 4)));
                m_Assembler.JnoJb(doneLabel);
                m_Assembler.CmovsEd(m_nRegisterLookupEx[tempRegister],
                    CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[negativeValueRegister]));
                m_Assembler.CmovnsEd(m_nRegisterLookupEx[tempRegister],
                    CX86Assembler::MakeRegisterAddress(m_nRegisterLookupEx[positiveValueRegister]));
                m_Assembler.MarkLabel(doneLabel);
                m_Assembler.Push(m_nRegisterLookupEx[tempRegister]);
            }

            FreeRegister(tempRegister);
            FreeRegister(positiveValueRegister);
            FreeRegister(negativeValueRegister);
        }
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

void CCodeGen::MD_AddS()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::AddpsVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_And()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::PandVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_CmpEqW()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::PcmpeqdVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_CmpGtH()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::PcmpgtwVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_IsNegative()
{
    if(FitsPattern<SingleRegister128>())
    {
        XMMREGISTER valueRegister = static_cast<XMMREGISTER>(GetPattern<SingleRegister128>());
        XMMREGISTER resultRegister;
        if(Register128HasNextUse(valueRegister))
        {
            resultRegister = AllocateXmmRegister();
            CopyRegister128(resultRegister, valueRegister);
        }
        else
        {
            resultRegister = valueRegister;
        }
        XMMREGISTER maskRegister = AllocateXmmRegister();
        m_Assembler.PcmpeqdVo(maskRegister,
            CX86Assembler::MakeXmmRegisterAddress(maskRegister));
        m_Assembler.PslldVo(maskRegister, 31);
        m_Assembler.PandVo(resultRegister,
            CX86Assembler::MakeXmmRegisterAddress(maskRegister));
        FreeXmmRegister(maskRegister);
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_IsZero()
{
    if(FitsPattern<SingleRegister128>())
    {
        XMMREGISTER valueRegister = static_cast<XMMREGISTER>(GetPattern<SingleRegister128>());
        XMMREGISTER resultRegister;
        if(Register128HasNextUse(valueRegister))
        {
            resultRegister = AllocateXmmRegister();
            CopyRegister128(resultRegister, valueRegister);
        }
        else
        {
            resultRegister = valueRegister;
        }
        XMMREGISTER zeroRegister = AllocateXmmRegister();
        m_Assembler.PandnVo(zeroRegister,
            CX86Assembler::MakeXmmRegisterAddress(zeroRegister));
        m_Assembler.PcmpeqdVo(resultRegister,
            CX86Assembler::MakeXmmRegisterAddress(zeroRegister));
        FreeXmmRegister(zeroRegister);
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }    
}

void CCodeGen::MD_MaxH()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::PmaxswVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_MaxS()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::MaxpsVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_MinH()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::PminswVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_MinS()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::MinpsVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_MulS()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::MulpsVo, m_Assembler, _1, _2));
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
    MD_GenericTwoOperand(bind(&CX86Assembler::PorVo, m_Assembler, _1, _2));
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
        m_Assembler.PackuswbVo(resultRegister,
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

void CCodeGen::MD_PackWH()
{
    if(FitsPattern<RelativeRelative128>())
    {
        RelativeRelative128::PatternValue ops(GetPattern<RelativeRelative128>());
        XMMREGISTER resultRegister = AllocateXmmRegister();
        XMMREGISTER tempRegister = AllocateXmmRegister();
        LoadRelative128InRegister(resultRegister, ops.second);
        LoadRelative128InRegister(tempRegister, ops.first);

        //Sign extend the lower half word of our registers
        m_Assembler.PslldVo(resultRegister, 0x10);
        m_Assembler.PsradVo(resultRegister, 0x10);

        m_Assembler.PslldVo(tempRegister, 0x10);
        m_Assembler.PsradVo(tempRegister, 0x10);

        //Pack
        m_Assembler.PackssdwVo(resultRegister,
            CX86Assembler::MakeXmmRegisterAddress(tempRegister));

        FreeXmmRegister(tempRegister);
        MD_PushReg(resultRegister);
    }
    else
    {
        throw exception();
    }
}

void CCodeGen::MD_SllH(uint8 amount)
{
    MD_GenericPackedShift(bind(&CX86Assembler::PsllwVo, &m_Assembler, _1, _2), amount);
}

void CCodeGen::MD_SraH(uint8 amount)
{
    MD_GenericPackedShift(bind(&CX86Assembler::PsrawVo, &m_Assembler, _1, _2), amount);
}

void CCodeGen::MD_SrlH(uint8 amount)
{
    MD_GenericPackedShift(bind(&CX86Assembler::PsrlwVo, &m_Assembler, _1, _2), amount);
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
    MD_GenericTwoOperand(bind(&CX86Assembler::PsubbVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_SubW()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::PsubdVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_SubS()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::SubpsVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_ToSingle()
{
    MD_GenericOneOperand(bind(&CX86Assembler::Cvtdq2psVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_ToWordTruncate()
{
    MD_GenericOneOperand(bind(&CX86Assembler::Cvttps2dqVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_UnpackLowerBH()
{
    MD_GenericTwoOperandReversed(bind(&CX86Assembler::PunpcklbwVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_UnpackLowerHW()
{
    MD_GenericTwoOperandReversed(bind(&CX86Assembler::PunpcklwdVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_UnpackLowerWD()
{
    MD_GenericTwoOperandReversed(bind(&CX86Assembler::PunpckldqVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_UnpackUpperBH()
{
    MD_GenericTwoOperandReversed(bind(&CX86Assembler::PunpckhbwVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_UnpackUpperWD()
{
    MD_GenericTwoOperandReversed(bind(&CX86Assembler::PunpckhdqVo, m_Assembler, _1, _2));
}

void CCodeGen::MD_Xor()
{
    MD_GenericTwoOperand(bind(&CX86Assembler::PxorVo, m_Assembler, _1, _2));
}
