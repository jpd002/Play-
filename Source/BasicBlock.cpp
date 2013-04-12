#include "BasicBlock.h"
#include "MemStream.h"
#include "offsetof_def.h"
#include "MipsJitter.h"
#include "Jitter_CodeGenFactory.h"

CBasicBlock::CBasicBlock(CMIPS& context, uint32 begin, uint32 end)
: m_begin(begin)
, m_end(end)
, m_context(context)
, m_selfLoopCount(0)
{
	assert(m_end >= m_begin);
}

CBasicBlock::~CBasicBlock()
{

}

void CBasicBlock::Compile()
{
	Framework::CMemStream stream;
	{
		static CMipsJitter* jitter = NULL;
		if(jitter == NULL)
		{
			Jitter::CCodeGen* codeGen = Jitter::CreateCodeGen();
			jitter = new CMipsJitter(codeGen);

			for(unsigned int i = 0; i < 4; i++)
			{
				jitter->SetVariableAsConstant(
					offsetof(CMIPS, m_State.nGPR[CMIPS::R0].nV[i]),
					0
					);
			}
		}

		jitter->SetStream(&stream);
		jitter->Begin();
		CompileRange(jitter);
//		codeGen.DumpVariables(0);
//		codeGen.EndQuota();
		jitter->End();
	}

	m_function = CMemoryFunction(stream.GetBuffer(), stream.GetSize());
}

void CBasicBlock::CompileRange(CMipsJitter* jitter)
{
	for(uint32 address = m_begin; address <= m_end; address += 4)
	{
		m_context.m_pArch->CompileInstruction(
			address, 
			jitter,
			&m_context);
		//Sanity check
		assert(jitter->IsStackEmpty());
	}
}

unsigned int CBasicBlock::Execute()
{
	m_function(&m_context);

	if(m_context.m_State.nDelayedJumpAddr != MIPS_INVALID_PC)
	{
		m_context.m_State.nPC = m_context.m_State.nDelayedJumpAddr;
		m_context.m_State.nDelayedJumpAddr = MIPS_INVALID_PC;
	}
	else
	{
		m_context.m_State.nPC = m_end + 4;
	}

	assert((m_context.m_State.nGPR[CMIPS::RA].nV0 & 3) == 0);
	assert(m_context.m_State.nCOP2[0].nV0 == 0x00000000);
	assert(m_context.m_State.nCOP2[0].nV1 == 0x00000000);
	assert(m_context.m_State.nCOP2[0].nV2 == 0x00000000);
	assert(m_context.m_State.nCOP2[0].nV3 == 0x3F800000);

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
	return !m_function.IsEmpty();
}

unsigned int CBasicBlock::GetSelfLoopCount() const
{
	return m_selfLoopCount;
}

void CBasicBlock::SetSelfLoopCount(unsigned int selfLoopCount)
{
	m_selfLoopCount = selfLoopCount;
}
