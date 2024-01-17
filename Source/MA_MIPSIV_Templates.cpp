#include "MA_MIPSIV.h"
#include "Jitter.h"
#include "MIPS.h"
#include "offsetof_def.h"

void CMA_MIPSIV::Template_Add32(bool isSigned)
{
	if(m_nRD == 0) return;

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	m_codeGen->Add();

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Add64(bool isSigned)
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Add64();
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Sub32(bool isSigned)
{
	if(m_nRD == 0) return;

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	m_codeGen->Sub();

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Sub64(bool isSigned)
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Sub64();
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Load32Idx(const MemoryAccessIdxTraits& traits)
{
	CheckTLBExceptions(false);

	if(m_nRT == 0) return;

	const auto finishLoad =
	    [&]() {
		    if(traits.signExtFunction)
		    {
			    //SignExt to 32-bits (if needed)
			    ((m_codeGen)->*(traits.signExtFunction))();
		    }

		    //Sign extend to whole 64-bits
		    if(m_regSize == MIPS_REGSIZE_64)
		    {
			    if(traits.upper64BitSignExtend)
			    {
				    m_codeGen->PushTop();
				    m_codeGen->SignExt();
			    }
			    else
			    {
				    m_codeGen->PushCst(0);
			    }
			    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		    }
		    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	    };

	bool usePageLookup = (m_pCtx->m_pageLookup != nullptr);

	if(usePageLookup)
	{
		ComputeMemAccessPageRef();

		m_codeGen->PushCst(0);
		m_codeGen->BeginIf(Jitter::CONDITION_NE);
		{
			ComputeMemAccessRefIdx(traits.elementSize);
			((m_codeGen)->*(traits.loadFunction))(1);
			finishLoad();
		}
		m_codeGen->Else();
	}

	//Standard memory access
	{
		ComputeMemAccessAddrNoXlat();

		m_codeGen->PushCtx();
		m_codeGen->PushIdx(1);
		m_codeGen->Call(traits.getProxyFunction, 2, Jitter::CJitter::RETURN_VALUE_32);

		finishLoad();

		m_codeGen->PullTop();
	}

	if(usePageLookup)
	{
		m_codeGen->EndIf();
	}
}

void CMA_MIPSIV::Template_Store32Idx(const MemoryAccessIdxTraits& traits)
{
	CheckTLBExceptions(true);

	bool usePageLookup = (m_pCtx->m_pageLookup != nullptr);

	if(usePageLookup)
	{
		ComputeMemAccessPageRef();

		m_codeGen->PushCst(0);
		m_codeGen->BeginIf(Jitter::CONDITION_NE);
		{
			ComputeMemAccessRefIdx(traits.elementSize);

			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
			((m_codeGen)->*(traits.storeFunction))(1);
		}
		m_codeGen->Else();
	}

	//Standard memory access
	{
		ComputeMemAccessAddrNoXlat();

		m_codeGen->PushCtx();
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PushIdx(2);
		m_codeGen->Call(traits.setProxyFunction, 3, Jitter::CJitter::RETURN_VALUE_NONE);

		m_codeGen->PullTop();
	}

	if(usePageLookup)
	{
		m_codeGen->EndIf();
	}
}

void CMA_MIPSIV::Template_ShiftCst32(const TemplateParamedOperationFunctionType& Function)
{
	if(m_nRD == 0) return;

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	Function(m_nSA);

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_ShiftVar32(const TemplateOperationFunctionType& function)
{
	if(m_nRD == 0) return;

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	function();

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Mult32(bool isSigned, unsigned int unit)
{
	auto function = isSigned ? &CMipsJitter::MultS : &CMipsJitter::Mult;

	size_t lo[2];
	size_t hi[2];

	switch(unit)
	{
	case 0:
		lo[0] = offsetof(CMIPS, m_State.nLO[0]);
		lo[1] = offsetof(CMIPS, m_State.nLO[1]);
		hi[0] = offsetof(CMIPS, m_State.nHI[0]);
		hi[1] = offsetof(CMIPS, m_State.nHI[1]);
		break;
	case 1:
		lo[0] = offsetof(CMIPS, m_State.nLO1[0]);
		lo[1] = offsetof(CMIPS, m_State.nLO1[1]);
		hi[0] = offsetof(CMIPS, m_State.nHI1[0]);
		hi[1] = offsetof(CMIPS, m_State.nHI1[1]);
		break;
	default:
		throw std::runtime_error("Invalid unit number.");
		break;
	}

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	((m_codeGen)->*(function))();

	m_codeGen->PushTop();

	m_codeGen->ExtLow64();
	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(lo[1]);
	}
	m_codeGen->PullRel(lo[0]);

	m_codeGen->ExtHigh64();
	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(hi[1]);
	}
	m_codeGen->PullRel(hi[0]);

	if(m_nRD != 0)
	{
		//Wierd EE MIPS spinoff...
		m_codeGen->PushRel(lo[0]);
		m_codeGen->PushRel(lo[1]);

		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
}

void CMA_MIPSIV::Template_Div32(bool isSigned, unsigned int unit, unsigned int regOffset)
{
	auto function = isSigned ? &CMipsJitter::DivS : &CMipsJitter::Div;

	size_t lo[2];
	size_t hi[2];

	switch(unit)
	{
	case 0:
		lo[0] = offsetof(CMIPS, m_State.nLO[0]);
		lo[1] = offsetof(CMIPS, m_State.nLO[1]);
		hi[0] = offsetof(CMIPS, m_State.nHI[0]);
		hi[1] = offsetof(CMIPS, m_State.nHI[1]);
		break;
	case 1:
		lo[0] = offsetof(CMIPS, m_State.nLO1[0]);
		lo[1] = offsetof(CMIPS, m_State.nLO1[1]);
		hi[0] = offsetof(CMIPS, m_State.nHI1[0]);
		hi[1] = offsetof(CMIPS, m_State.nHI1[1]);
		break;
	default:
		throw std::runtime_error("Invalid unit number.");
		break;
	}

	//Check for zero
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[regOffset]));
	m_codeGen->PushCst(0);
	m_codeGen->BeginIf(Jitter::CONDITION_EQ);
	{
		if(isSigned)
		{
			//If r[rs] < 0, then lo = 1 else lo = ~0
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[regOffset]));
			m_codeGen->PushCst(0);
			m_codeGen->BeginIf(Jitter::CONDITION_LT);
			{
				m_codeGen->PushCst(1);
				m_codeGen->PullRel(lo[0]);
			}
			m_codeGen->Else();
			{
				m_codeGen->PushCst(~0);
				m_codeGen->PullRel(lo[0]);
			}
			m_codeGen->EndIf();
		}
		else
		{
			m_codeGen->PushCst(~0);
			m_codeGen->PullRel(lo[0]);
		}

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[regOffset]));
		m_codeGen->PullRel(hi[0]);
	}
	m_codeGen->Else();
	{
		if(isSigned)
		{
			//Check for overflow condition (0x80000000 / 0xFFFFFFFF)
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[regOffset]));
			m_codeGen->PushCst(0x80000000);
			m_codeGen->Cmp(Jitter::CONDITION_EQ);

			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[regOffset]));
			m_codeGen->PushCst(0xFFFFFFFF);
			m_codeGen->Cmp(Jitter::CONDITION_EQ);

			m_codeGen->And();
		}
		else
		{
			m_codeGen->PushCst(0);
		}

		m_codeGen->PushCst(0);

		m_codeGen->BeginIf(Jitter::CONDITION_NE);
		{
			//Overflow
			m_codeGen->PushCst(0x80000000);
			m_codeGen->PullRel(lo[0]);

			m_codeGen->PushCst(0);
			m_codeGen->PullRel(hi[0]);
		}
		m_codeGen->Else();
		{
			//No overflow, carry on
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[regOffset]));
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[regOffset]));
			((m_codeGen)->*(function))();

			m_codeGen->PushTop();

			m_codeGen->ExtLow64();
			m_codeGen->PullRel(lo[0]);

			m_codeGen->ExtHigh64();
			m_codeGen->PullRel(hi[0]);
		}
		m_codeGen->EndIf();
	}
	m_codeGen->EndIf();

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushRel(hi[0]);
		m_codeGen->SignExt();
		m_codeGen->PullRel(hi[1]);

		m_codeGen->PushRel(lo[0]);
		m_codeGen->SignExt();
		m_codeGen->PullRel(lo[1]);
	}
}

void CMA_MIPSIV::Template_MovEqual(bool isEqual)
{
	if(m_nRD == 0) return;

	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	}
	else
	{
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PushCst64(0);
		m_codeGen->Cmp64(Jitter::CONDITION_NE);
	}

	m_codeGen->PushCst(0);
	m_codeGen->BeginIf(isEqual ? Jitter::CONDITION_EQ : Jitter::CONDITION_NE);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

		if(m_regSize == MIPS_REGSIZE_64)
		{
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		}
	}
	m_codeGen->EndIf();
}

void CMA_MIPSIV::Template_SetLessThanImm(bool isSigned)
{
	Jitter::CONDITION condition = isSigned ? Jitter::CONDITION_LT : Jitter::CONDITION_BL;

	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushCst(static_cast<int16>(m_nImmediate));

		m_codeGen->Cmp(condition);
	}
	else
	{
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushCst64(static_cast<int16>(m_nImmediate));
		m_codeGen->Cmp64(condition);
	}

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	if(m_regSize == MIPS_REGSIZE_64)
	{
		//Clear higher 32-bits
		m_codeGen->PushCst(0);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
}

void CMA_MIPSIV::Template_SetLessThanReg(bool isSigned)
{
	Jitter::CONDITION condition = isSigned ? Jitter::CONDITION_LT : Jitter::CONDITION_BL;

	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->Cmp(condition);
	}
	else
	{
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->Cmp64(condition);
	}

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	if(m_regSize == MIPS_REGSIZE_64)
	{
		//Clear higher 32-bits
		m_codeGen->PushCst(0);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}
}

void CMA_MIPSIV::Template_BranchEq(bool condition, bool likely)
{
	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	}
	else if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->Cmp64(Jitter::CONDITION_NE);

		m_codeGen->PushCst(0);
	}

	auto branchCondition = condition ? Jitter::CONDITION_EQ : Jitter::CONDITION_NE;

	if(likely)
	{
		BranchLikely(branchCondition);
	}
	else
	{
		Branch(branchCondition);
	}
}

void CMA_MIPSIV::Template_BranchGez(bool condition, bool likely)
{
	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	}
	else
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
	}

	m_codeGen->PushCst(0x80000000);
	m_codeGen->And();

	m_codeGen->PushCst(0);

	Jitter::CONDITION branchCondition = condition ? Jitter::CONDITION_EQ : Jitter::CONDITION_NE;

	if(likely)
	{
		BranchLikely(branchCondition);
	}
	else
	{
		Branch(branchCondition);
	}
}

void CMA_MIPSIV::Template_BranchLez(bool condition, bool likely)
{
	Jitter::CONDITION branchCondition;

	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushCst(0);

		branchCondition = condition ? Jitter::CONDITION_LE : Jitter::CONDITION_GT;
	}
	else
	{
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushCst64(0);
		m_codeGen->Cmp64(condition ? Jitter::CONDITION_LE : Jitter::CONDITION_GT);

		m_codeGen->PushCst(0);
		branchCondition = Jitter::CONDITION_NE;
	}

	if(likely)
	{
		BranchLikely(branchCondition);
	}
	else
	{
		Branch(branchCondition);
	}
}
