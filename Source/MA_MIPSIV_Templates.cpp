#include "MA_MIPSIV.h"
#include "CodeGen.h"
#include "MIPS.h"
#include "offsetof_def.h"

using namespace std;

void CMA_MIPSIV::Template_Add32(bool isSigned)
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->Add();

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->SeX();
	    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Add64(bool isSigned)
{
	assert(m_regSize == MIPS_REGSIZE_64);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Add64();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Sub32(bool isSigned)
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->Sub();

	if(m_regSize == MIPS_REGSIZE_64)
	{
	    m_codeGen->SeX();
	    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_LoadUnsigned32(void* pProxyFunction)
{
    //TODO: Need to check if this used correctly... LBU, LHU and LW uses this (why LW? and why sign extend on LBU and LHU?)

    ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushIdx(1);
	m_codeGen->Call(pProxyFunction, 2, true);

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->SeX();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->PullTop();
}

void CMA_MIPSIV::Template_ShiftCst32(const TemplateParamedOperationFunctionType& Function)
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    Function(m_nSA);
    
	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->SeX();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_ShiftVar32(const TemplateOperationFunctionType& function)
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    function();

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->SeX();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Mult32(const TemplateOperationFunctionType& Function, unsigned int unit)
{
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
        throw runtime_error("Invalid unit number.");
        break;
    }

    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    Function();

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->SeX();
		m_codeGen->PullRel(lo[1]);
	}
    m_codeGen->PullRel(lo[0]);

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->SeX();
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

void CMA_MIPSIV::Template_Div32(const TemplateOperationFunctionType& function, unsigned int unit)
{
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
        throw runtime_error("Invalid unit number.");
        break;
    }

	//Check for zero
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->BeginIfElse(false);
	{
		m_codeGen->PushCst(~0);

		m_codeGen->PushTop();
		m_codeGen->PullRel(lo[1]);

		m_codeGen->PushTop();
		m_codeGen->PullRel(lo[0]);

		m_codeGen->PushTop();
		m_codeGen->PullRel(hi[1]);

		m_codeGen->PullRel(hi[0]);
	}
	m_codeGen->BeginIfElseAlt();
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		function();

		if(m_regSize == MIPS_REGSIZE_64)
		{
			m_codeGen->SeX();
			m_codeGen->PullRel(lo[1]);
		}
		m_codeGen->PullRel(lo[0]);

		if(m_regSize == MIPS_REGSIZE_64)
		{
			m_codeGen->SeX();
			m_codeGen->PullRel(hi[1]);
		}
		m_codeGen->PullRel(hi[0]);
	}
	m_codeGen->EndIf();
}

void CMA_MIPSIV::Template_MovEqual(bool isEqual)
{
	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PushCst(0);
		m_codeGen->Cmp(CCodeGen::CONDITION_EQ);
	}
	else
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		m_codeGen->PushCst(0);
		m_codeGen->PushCst(0);

		m_codeGen->Cmp64(CCodeGen::CONDITION_EQ);
	}

    m_codeGen->BeginIf(isEqual);
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
	CCodeGen::CONDITION condition = isSigned ? CCodeGen::CONDITION_LT : CCodeGen::CONDITION_BL;

	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushCst(static_cast<int16>(m_nImmediate));

		m_codeGen->Cmp(condition);
	}
	else
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		m_codeGen->PushCst(static_cast<int16>(m_nImmediate));
		m_codeGen->PushCst(m_nImmediate & 0x8000 ? 0xFFFFFFFF : 0x00000000);

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
	CCodeGen::CONDITION condition = isSigned ? CCodeGen::CONDITION_LT : CCodeGen::CONDITION_BL;

	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->Cmp(condition);
	}
	else
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

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

		m_codeGen->Cmp(CCodeGen::CONDITION_EQ);
	}
	else if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		m_codeGen->Cmp64(CCodeGen::CONDITION_EQ);
	}

    if(likely)
    {
        BranchLikely(condition);
    }
    else
    {
        Branch(condition);
    }
}

void CMA_MIPSIV::Template_BranchGez(bool condition, bool likely)
{
    m_codeGen->PushCst(0);

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

    m_codeGen->Cmp(CCodeGen::CONDITION_EQ);

    if(likely)
    {
        BranchLikely(condition);
    }
    else
    {
        Branch(condition);
    }
}

void CMA_MIPSIV::Template_BranchLez(bool condition, bool likely)
{
	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushCst(0);
		m_codeGen->Cmp(CCodeGen::CONDITION_LE);
	}
	else
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		m_codeGen->PushCst(0);
		m_codeGen->PushCst(0);

		m_codeGen->Cmp64(CCodeGen::CONDITION_LE);
	}

    if(likely)
    {
        BranchLikely(condition);
    }
    else
    {
        Branch(condition);
    }
}
