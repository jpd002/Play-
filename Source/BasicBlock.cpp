#include "BasicBlock.h"
#include "MipsCodeGen.h"
#include "MemStream.h"

using namespace Framework;

CBasicBlock::CBasicBlock(CMIPS& context, uint32 begin, uint32 end) :
m_begin(begin),
m_end(end),
m_context(context),
m_text(NULL)
{
    assert(m_end > m_begin);
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
    codeGen.DumpVariables();
    codeGen.m_Assembler.Ret();
    codeGen.End();

    //Save text
    m_text = new uint8[stream.GetSize()];
    memcpy(m_text, stream.GetBuffer(), stream.GetSize());
}

void CBasicBlock::Execute()
{
    void* function = m_text;
    CMIPS* context = &m_context;

    __asm
    {
        push    ebp

        mov     eax, [function] 
		mov		ebp, [context]
        call    eax

        pop     ebp
    }
}
