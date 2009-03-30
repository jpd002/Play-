#include "BasicBlock.h"
#include "MipsCodeGen.h"
#include "MemStream.h"
#include "offsetof_def.h"

using namespace Framework;

CBasicBlock::CBasicBlock(CMIPS& context, uint32 begin, uint32 end) :
m_begin(begin),
m_end(end),
m_context(context),
m_text(NULL),
m_selfLoopCount(0),
m_branchHint(NULL)
{
    assert(m_end >= m_begin);
}

CBasicBlock::~CBasicBlock()
{
    if(m_text != NULL)
    {
        delete [] m_text;
    }
}

void CBasicBlock::Compile()
{
    CMemStream stream;
    {
        CMipsCodeGen codeGen;

        for(unsigned int i = 0; i < 4; i++)
        {
            codeGen.SetVariableAsConstant(
                offsetof(CMIPS, m_State.nGPR[CMIPS::R0].nV[i]),
                0
                );
        }
        codeGen.SetStream(&stream);
        codeGen.Begin();
        for(uint32 address = m_begin; address <= m_end; address += 4)
        {
            m_context.m_pArch->CompileInstruction(
                address, 
                &codeGen,
                &m_context);
            //Sanity check
            assert(codeGen.IsStackEmpty());
        }
        codeGen.DumpVariables(0);
        codeGen.m_Assembler.Ret();
        codeGen.End();
    }

    //Save text
    m_text = new uint8[stream.GetSize()];
    memcpy(m_text, stream.GetBuffer(), stream.GetSize());
}

unsigned int CBasicBlock::Execute()
{
    volatile void* function = m_text;
    volatile CMIPS* context = &m_context;

#ifdef AMD64

    

#else

    //Should change pre-proc definitions used here (MACOSX to GCC?)
    __asm
    {
#if defined(MACOSX)
		mov eax, esp
		sub eax, 0x04
		and eax, 0x0F
		sub esp, eax		//Keep stack aligned on 0x10 bytes
		pusha
#elif defined(_MSVC)
        pushad
#endif

        mov     eax, function
		mov		ebp, context
        call    eax
		
#if defined(MACOSX)
		popa
		add esp, eax
#elif defined(_MSVC)
        popad
#endif
    }
#endif

    if((m_context.m_State.nGPR[CMIPS::RA].nV0 & 3) != 0)
    {
        assert(0);
    }

    if(m_context.m_State.nDelayedJumpAddr != MIPS_INVALID_PC)
    {
        m_context.m_State.nPC = m_context.m_State.nDelayedJumpAddr;
        m_context.m_State.nDelayedJumpAddr = MIPS_INVALID_PC;
    }
    else
    {
        m_context.m_State.nPC = m_end + 4;
    }

//    if(m_context.m_State.pipeQ.target != MIPS_INVALID_PC)
//    {
//        uint32& target = m_context.m_State.pipeQ.target;
//        if(m_end >= target)
//        {
//            m_context.m_State.nCOP2Q = m_context.m_State.pipeQ.heldValue;
//            target = MIPS_INVALID_PC;
//        }
//        else
//        {
//            uint32 remain = target - m_end;
//            target = m_context.m_State.nPC + remain - 4;
//        }
//    }

    return ((m_end - m_begin) / 4) + 1;
}

uint32 CBasicBlock::GetBeginAddress() const
{
    return m_begin;
}

uint32 CBasicBlock::GetEndAddress() const
{
    return m_end;
}

bool CBasicBlock::IsCompiled() const
{
    return m_text != NULL;
}

unsigned int CBasicBlock::GetSelfLoopCount() const
{
    return m_selfLoopCount;
}

void CBasicBlock::SetSelfLoopCount(unsigned int selfLoopCount)
{
    m_selfLoopCount = selfLoopCount;
}

CBasicBlock* CBasicBlock::GetBranchHint() const
{
    return m_branchHint;
}

void CBasicBlock::SetBranchHint(CBasicBlock* branchHint)
{
    m_branchHint = branchHint;
}
