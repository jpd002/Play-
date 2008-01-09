#include "BasicBlock.h"
#include "MipsCodeGen.h"
#include "MemStream.h"
#include "offsetof_def.h"

using namespace Framework;

CBasicBlock::CBasicBlock(CMIPS& context, uint32 begin, uint32 end) :
m_begin(begin),
m_end(end),
m_context(context),
m_text(NULL)
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
        codeGen.Begin(NULL);
        for(uint32 address = m_begin; address <= m_end; address += 4)
        {
            m_context.m_pArch->CompileInstruction(
                address, 
                reinterpret_cast<CCacheBlock*>(&codeGen),
                &m_context,
                true);
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

    __asm
    {
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi
        push    ebp
		
        mov     eax, function
		mov		ebp, context
        call    eax
		
        pop     ebp
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
    }

//	asm("pushl %%ebx\n\t"
//		"movl %%edi, %%eax\n\t"
//		"movl %%ecx, %%ebp\n\t"
//		"pushl %%eax\n\t"
//		"call (%%esp)\n\t"
//		"popl %%eax\n\t"
//		"popl %%ebx\n\t"
//		:
//		: "D" (function), "c" (context)
//		: "%ebp", "%esi");

    if(m_context.m_State.nDelayedJumpAddr != MIPS_INVALID_PC)
    {
        m_context.m_State.nPC = m_context.m_State.nDelayedJumpAddr;
        m_context.m_State.nDelayedJumpAddr = MIPS_INVALID_PC;
    }
    else
    {
        m_context.m_State.nPC = m_end + 4;
    }

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
