#include "VuBasicBlock.h"
#include "MA_VU.h"

CVuBasicBlock::CVuBasicBlock(CMIPS& context, uint32 begin, uint32 end) :
CBasicBlock(context, begin, end)
{

}

CVuBasicBlock::~CVuBasicBlock()
{

}

void CVuBasicBlock::CompileRange(CCodeGen& codeGen)
{
	assert((m_begin & 0x07) == 0);
	assert(((m_end + 4) & 0x07) == 0);
	CMA_VU* arch = static_cast<CMA_VU*>(m_context.m_pArch);
	for(uint32 address = m_begin; address <= m_end; address += 8)
	{
		uint32 addressLo = address + 0;
		uint32 addressHi = address + 4;

		uint32 opcodeLo = m_context.m_pMemoryMap->GetInstruction(addressLo);
		uint32 opcodeHi = m_context.m_pMemoryMap->GetInstruction(addressHi);

		VUShared::OPERANDSET loOps = arch->GetAffectedOperands(&m_context, addressLo, opcodeLo);
		VUShared::OPERANDSET hiOps = arch->GetAffectedOperands(&m_context, addressHi, opcodeHi);

		if(opcodeHi & 0x80000000)
		{
			arch->CompileInstruction(addressHi, &codeGen, &m_context);
		}
		else
		{
			uint8 savedReg = 0;

			if(hiOps.writeF != 0)
			{
				assert(hiOps.writeF != loOps.writeF);
				if(
					(hiOps.writeF == loOps.readF0) ||
					(hiOps.writeF == loOps.readF1)
					)
				{
					savedReg = hiOps.writeF;
					codeGen.MD_PushRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
					codeGen.MD_PullRel(offsetof(CMIPS, m_State.nCOP2VF_PreUp));
				}
			}

			arch->CompileInstruction(addressHi, &codeGen, &m_context);

			if(savedReg != 0)
			{
				codeGen.MD_PushRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
				codeGen.MD_PullRel(offsetof(CMIPS, m_State.nCOP2VF_UpRes));

				codeGen.MD_PushRel(offsetof(CMIPS, m_State.nCOP2VF_PreUp));
				codeGen.MD_PullRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
			}

			arch->CompileInstruction(addressLo, &codeGen, &m_context);

			if(savedReg != 0)
			{
				codeGen.MD_PushRel(offsetof(CMIPS, m_State.nCOP2VF_UpRes));
				codeGen.MD_PullRel(offsetof(CMIPS, m_State.nCOP2[savedReg]));
			}
		}

		//Sanity check
		assert(codeGen.IsStackEmpty());
	}
}
