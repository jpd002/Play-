#include "BasicBlock.h"
#include "MipsCodeGen.h"
#include "MemStream.h"
#include "offsetof_def.h"
#ifdef ARM
#include <mach/mach_init.h>
#include <mach/vm_map.h>
extern "C" void __clear_cache(void* begin, void* end);
#endif

using namespace Framework;

CBasicBlock::CBasicBlock(CMIPS& context, uint32 begin, uint32 end) :
m_begin(begin),
m_end(end),
m_context(context),
m_text(NULL),
m_textSize(0),
m_selfLoopCount(0),
m_branchHint(NULL)
{
    assert(m_end >= m_begin);
}

CBasicBlock::~CBasicBlock()
{
    if(m_text != NULL)
    {
#ifdef ARM
		vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(m_text), m_textSize); 
#else
        delete [] m_text;
#endif
		
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
		CompileRange(codeGen);
        codeGen.DumpVariables(0);
		codeGen.EndQuota();
        codeGen.End();
    }

#ifdef ARM
	vm_size_t page_size = 0;
	host_page_size(mach_task_self(), &page_size);
	unsigned int allocSize = ((stream.GetSize() + page_size - 1) / page_size) * page_size;
	vm_allocate(mach_task_self(), reinterpret_cast<vm_address_t*>(&m_text), allocSize, TRUE); 
	memcpy(m_text, stream.GetBuffer(), stream.GetSize());
	__clear_cache(m_text, m_text + stream.GetSize());
	kern_return_t result = vm_protect(mach_task_self(), reinterpret_cast<vm_address_t>(m_text), 
									  stream.GetSize(), 0, VM_PROT_READ | VM_PROT_EXECUTE);
	assert(result == 0);
	m_textSize = allocSize;
#else
    //Save text
    m_text = new uint8[stream.GetSize()];
	m_textSize = stream.GetSize();
    memcpy(m_text, stream.GetBuffer(), stream.GetSize());
#endif
}

void CBasicBlock::CompileRange(CCodeGen& codeGen)
{
	for(uint32 address = m_begin; address <= m_end; address += 4)
	{
		m_context.m_pArch->CompileInstruction(
			address, 
			&codeGen,
			&m_context);
		//Sanity check
		//assert(codeGen.AreAllRegistersFreed());
		assert(codeGen.IsStackEmpty());
	}
}

unsigned int CBasicBlock::Execute()
{
    volatile void* function = m_text;
    volatile CMIPS* context = &m_context;

#ifdef AMD64

#elif defined(ARM)
	
	__asm__ ("mov r1, %0" : : "r"(context) : "r1");
	__asm__ ("mov r0, %0" : : "r"(function) : "r0");
	__asm__ ("stmdb sp!, {r2, r3, r4, r5, r6, r7, r11, ip}");
	__asm__ ("mov r11, r1");
	__asm__ ("blx r0");
	__asm__ ("ldmia sp!, {r2, r3, r4, r5, r6, r7, r11, ip}");
	
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
