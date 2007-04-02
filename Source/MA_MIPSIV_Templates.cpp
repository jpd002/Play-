#include "MA_MIPSIV.h"
#include "CodeGen.h"
#include "MIPS.h"

void CMA_MIPSIV::Template_LoadUnsigned32::operator()(void* pProxyFunction)
{
    CCodeGen::Begin(m_pB);
    {
        ComputeMemAccessAddrEx();

		CCodeGen::PushRef(m_pCtx);
		CCodeGen::PushIdx(1);
		CCodeGen::Call(pProxyFunction, 2, true);

		CCodeGen::SeX();
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

        CCodeGen::PullTop();
    }
    CCodeGen::End();
}

void CMA_MIPSIV::Template_ShiftCst32::operator()(OperationFunctionType Function)
{
    CCodeGen::Begin(m_pB);
    {
        CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
        Function(m_nSA);
        
        CCodeGen::SeX();
        CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
        CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
    }
    CCodeGen::End();
}

void CMA_MIPSIV::Template_Mult32::operator()(OperationFunctionType Function)
{
    CCodeGen::Begin(m_pB);
    {
        CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
        CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
        Function();

        CCodeGen::SeX();
        CCodeGen::PullRel(offsetof(CMIPS, m_State.nLO[1]));
        CCodeGen::PullRel(offsetof(CMIPS, m_State.nLO[0]));

        CCodeGen::SeX();
        CCodeGen::PullRel(offsetof(CMIPS, m_State.nHI[1]));
        CCodeGen::PullRel(offsetof(CMIPS, m_State.nHI[0]));

        if(m_nRD != 0)
        {
    		//Wierd EE MIPS spinoff...
            CCodeGen::PushRel(offsetof(CMIPS, m_State.nLO[0]));
            CCodeGen::PushRel(offsetof(CMIPS, m_State.nLO[1]));

            CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
            CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
        }
    }
    CCodeGen::End();
}
