#include "MA_MIPSIV.h"
#include "CodeGen.h"
#include "MipsCodeGen.h"
#include "MIPS.h"
#include "offsetof_def.h"

void CMA_MIPSIV::Template_LoadUnsigned32::operator()(void* pProxyFunction)
{
    ComputeMemAccessAddrEx();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushIdx(1);
	m_codeGen->Call(pProxyFunction, 2, true);

	m_codeGen->SeX();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->PullTop();
}

void CMA_MIPSIV::Template_ShiftCst32::operator()(OperationFunctionType Function)
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    Function(m_nSA);
    
    m_codeGen->SeX();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

void CMA_MIPSIV::Template_Mult32::operator()(OperationFunctionType Function)
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    Function();

    m_codeGen->SeX();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO[0]));

    m_codeGen->SeX();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI[0]));

    if(m_nRD != 0)
    {
		//Wierd EE MIPS spinoff...
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO[0]));
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO[1]));

        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
    }
}

void CMA_MIPSIV::Template_MovEqual::operator()(bool isEqual) const
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

    m_codeGen->PushCst(0);
    m_codeGen->PushCst(0);

    m_codeGen->Cmp64(CCodeGen::CONDITION_EQ);

    m_codeGen->BeginIf(isEqual);
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    }
    m_codeGen->EndIf();
}

void CMA_MIPSIV::Template_BranchGez::operator()(bool condition, bool likely) const
{
    m_codeGen->PushCst(0);

    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
    m_codeGen->PushCst(0x80000000);
    m_codeGen->And();

    m_codeGen->Cmp(CCodeGen::CONDITION_EQ);

    if(likely)
    {
        BranchLikelyEx(condition);
    }
    else
    {
        BranchEx(condition);
    }
}

void CMA_MIPSIV::Template_BranchLez::operator()(bool condition, bool likely) const
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

    m_codeGen->PushCst(0);
    m_codeGen->PushCst(0);

    m_codeGen->Cmp64(CCodeGen::CONDITION_LE);

    if(likely)
    {
        BranchLikelyEx(condition);
    }
    else
    {
        BranchEx(condition);
    }
}
