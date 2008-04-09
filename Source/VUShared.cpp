#include "VUShared.h"
#include "MIPS.h"
#include "offsetof_def.h"

using namespace VUShared;
using namespace std;

bool VUShared::DestinationHasElement(uint8 nDest, unsigned int nElement)
{
	return (nDest & (1 << (nElement ^ 0x03))) != 0;
}

uint32* VUShared::GetVectorElement(CMIPS* pCtx, unsigned int nReg, unsigned int nElement)
{
	switch(nElement)
	{
	case 0:
		return &pCtx->m_State.nCOP2[nReg].nV0;
		break;
	case 1:
		return &pCtx->m_State.nCOP2[nReg].nV1;
		break;
	case 2:
		return &pCtx->m_State.nCOP2[nReg].nV2;
		break;
	case 3:
		return &pCtx->m_State.nCOP2[nReg].nV3;
		break;
	}
	return NULL;
}

size_t VUShared::GetVectorElement(unsigned int nRegister, unsigned int nElement)
{
	return offsetof(CMIPS, m_State.nCOP2[nRegister].nV[nElement]);
}

uint32* VUShared::GetAccumulatorElement(CMIPS* pCtx, unsigned int nElement)
{
	switch(nElement)
	{
	case 0:
		return &pCtx->m_State.nCOP2A.nV0;
		break;
	case 1:
		return &pCtx->m_State.nCOP2A.nV1;
		break;
	case 2:
		return &pCtx->m_State.nCOP2A.nV2;
		break;
	case 3:
		return &pCtx->m_State.nCOP2A.nV3;
		break;
	}
	return NULL;	
}

size_t VUShared::GetAccumulatorElement(unsigned int nElement)
{
	return offsetof(CMIPS, m_State.nCOP2A.nV[nElement]);
}

void VUShared::PullVector(CCodeGen* codeGen, uint8 dest, size_t vector)
{
    codeGen->MD_PullRel(
        DestinationHasElement(dest, 0) ? vector + 0x0 : SIZE_MAX,
        DestinationHasElement(dest, 1) ? vector + 0x4 : SIZE_MAX,
        DestinationHasElement(dest, 2) ? vector + 0x8 : SIZE_MAX,
        DestinationHasElement(dest, 3) ? vector + 0xC : SIZE_MAX);
}

void VUShared::ABS(CCodeGen* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_AbsS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::ADD(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_AddS();

    //Get result flags
    codeGen->PushTop();
    codeGen->PushTop();

    codeGen->MD_IsNegative();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2SF));

    codeGen->MD_IsZero();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2ZF));

    //Save result
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::ADDbc(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::ADDi(CCodeGen* codeGen, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(pB);
	//{
	//	CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
	//	CVUF128::Push(&pCtx->m_State.nCOP2I);
	//	CVUF128::Add();
	//	PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	//}
	//CCodeGen::End();
}

void VUShared::ADDq(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2Q));
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::ADDAbc(CCodeGen* codeGen, CMIPS* pCtx, uint8 nDest, uint8 nFs, uint8 nFt, uint8 nBc)
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(pB);
	//{
	//	CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
	//	CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
	//	CVUF128::Add();
	//	PullVector(nDest, &pCtx->m_State.nCOP2A);
	//}
	//CCodeGen::End();
}

void VUShared::CLIP(CCodeGen* codeGen, CMIPS* pCtx, uint8 nFs, uint8 nFt)
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(pB);
	//{
	//	//Create some space for the new test results
	//	CCodeGen::PushVar(&pCtx->m_State.nCOP2CF);
	//	//Decomment -> CCodeGen::Shl(6);
	//	CCodeGen::PullVar(&pCtx->m_State.nCOP2CF);

	//	for(unsigned int i = 0; i < 3; i++)
	//	{
	//		//c > +|w|
	//		CFPU::PushSingle(&pCtx->m_State.nCOP2[nFt].nV[3]);
	//		CFPU::Abs();
	//		CFPU::PushSingle(&pCtx->m_State.nCOP2[nFs].nV[i]);
	//		CFPU::Cmp(CCodeGen::CONDITION_AB);
	//		
	//		CCodeGen::BeginIf(true);
	//		{
	//			CCodeGen::PushVar(&pCtx->m_State.nCOP2CF);
	//			CCodeGen::PushCst(1 << ((i * 2) + 0));
	//			//Decomment -> CCodeGen::Or();
	//			CCodeGen::PullVar(&pCtx->m_State.nCOP2CF);
	//		}
	//		CCodeGen::EndIf();

	//		//c < -|w|
	//		CFPU::PushSingle(&pCtx->m_State.nCOP2[nFt].nV[3]);
	//		CFPU::Abs();
	//		CFPU::Neg();
	//		CFPU::PushSingle(&pCtx->m_State.nCOP2[nFs].nV[i]);
	//		CFPU::Cmp(CCodeGen::CONDITION_BL);

	//		CCodeGen::BeginIf(true);
	//		{
	//			CCodeGen::PushVar(&pCtx->m_State.nCOP2CF);
	//			CCodeGen::PushCst(1 << ((i * 2) + 1));
	//			//Decomment -> CCodeGen::Or();
	//			CCodeGen::PullVar(&pCtx->m_State.nCOP2CF);
	//		}
	//		CCodeGen::EndIf();
	//	}

	//}
	//CCodeGen::End();
}

void VUShared::DIV(CCodeGen* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf)
{
    //Check for zero
    codeGen->PushRel(GetVectorElement(nFt, nFtf));
    codeGen->PushCst(0x7FFFFFFF);
    codeGen->And();
    codeGen->PushCst(0);
    codeGen->Cmp(CCodeGen::CONDITION_EQ);

    codeGen->BeginIfElse(true);
    {
        codeGen->PushCst(0x7F7FFFFF);
        codeGen->PushRel(GetVectorElement(nFs, nFsf));
        codeGen->PushRel(GetVectorElement(nFt, nFtf));
        codeGen->Xor();
        codeGen->PushCst(0x80000000);
        codeGen->And();
        codeGen->Or();
        codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2Q));
    }
    codeGen->BeginIfElseAlt();
    {
        codeGen->FP_PushSingle(GetVectorElement(nFs, nFsf));
        codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
        codeGen->FP_Div();
        codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP2Q));
    }
    codeGen->EndIf();
}

void VUShared::FTOI0(CCodeGen* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_ToWordTruncate();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::FTOI4(CCodeGen* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushCstExpand(16.0f);
    codeGen->MD_MulS();
    codeGen->MD_ToWordTruncate();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::ITOF0(CCodeGen* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_ToSingle();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::MADD(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MADDbc(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MADDA(CCodeGen* codeGen, uint8 nDest, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MADDAbc(CCodeGen* codeGen, uint8 nDest, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MADDAi(CCodeGen* codeGen, uint8 nDest, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MAX(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MaxS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MAXbc(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MaxS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MINI(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MinS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MINIbc(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MinS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MINIi(CCodeGen* codeGen, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(pB);
	//{
	//	CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
	//	CVUF128::Push(&pCtx->m_State.nCOP2I);
	//	CVUF128::Min();
	//	PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	//}
	//CCodeGen::End();
}

void VUShared::MOVE(CCodeGen* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    for(unsigned int i = 0; i < 4; i++)
    {
	    if(!DestinationHasElement(nDest, i)) continue;

	    codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
	    codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[nFt].nV[i]));
    }
}

void VUShared::MR32(CCodeGen* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    for(unsigned int i = 0; i < 4; i++)
    {
        if(!DestinationHasElement(nDest, i)) continue;

        codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[(i + 1) & 3]));
        codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[nFt].nV[i]));
    }
}

void VUShared::MSUBi(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    codeGen->MD_SubS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MSUBAi(CCodeGen* codeGen, uint8 nDest, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    codeGen->MD_SubS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MUL(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULbc(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULi(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULq(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2Q));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULA(CCodeGen* codeGen, uint8 nDest, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MULAbc(CCodeGen* codeGen, uint8 nDest, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MULAi(CCodeGen* codeGen, uint8 nDest, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::OPMULA(CCodeGen* codeGen, uint8 nFs, uint8 nFt)
{
    //ACCx
    codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPY));
    codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPZ));
    codeGen->FP_Mul();
    codeGen->FP_PullSingle(GetAccumulatorElement(VECTOR_COMPX));

    //ACCy
    codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPZ));
    codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPX));
    codeGen->FP_Mul();
    codeGen->FP_PullSingle(GetAccumulatorElement(VECTOR_COMPY));

    //ACCz
    codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPX));
    codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPY));
    codeGen->FP_Mul();
    codeGen->FP_PullSingle(GetAccumulatorElement(VECTOR_COMPZ));
}

void VUShared::OPMSUB(CCodeGen* codeGen, uint8 nFd, uint8 nFs, uint8 nFt)
{
	if(nFd == 0) 
	{
		//Atelier Iris - OPMSUB with VF0 as FD...
		//This is probably to set a flag which is tested a bit further
		//The flag tested is Sz
        throw runtime_error("Remimplement.");
        /*
		CCodeGen::Begin(pB);
		{
			CFPU::PushSingleImm(0.0);

			CFPU::PushSingle(GetAccumulatorElement(pCtx, VECTOR_COMPZ));
			CFPU::PushSingle(GetVectorElement(pCtx, nFs, VECTOR_COMPX));
			CFPU::PushSingle(GetVectorElement(pCtx, nFt, VECTOR_COMPY));
			CFPU::Mul();
			CFPU::Sub();

			CFPU::Cmp(CCodeGen::CONDITION_BL);
			
			CCodeGen::BeginIfElse(true);
			{
				CCodeGen::PushCst(0xFFFFFFFF);
				CCodeGen::PullVar(&pCtx->m_State.nCOP2SF.nV[2]);
			}
			CCodeGen::BeginIfElseAlt();
			{
				CCodeGen::PushCst(0);
				CCodeGen::PullVar(&pCtx->m_State.nCOP2SF.nV[2]);
			}
			CCodeGen::EndIf();
		}
		CCodeGen::End();
		return;
        */
	}

    //X
    codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPX));
    codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPY));
    codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPZ));
    codeGen->FP_Mul();
    codeGen->FP_Sub();
    codeGen->FP_PullSingle(GetVectorElement(nFd, VECTOR_COMPX));

    //Y
    codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPY));
    codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPZ));
    codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPX));
    codeGen->FP_Mul();
    codeGen->FP_Sub();
    codeGen->FP_PullSingle(GetVectorElement(nFd, VECTOR_COMPY));

    //Z
    codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPZ));
    codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPX));
    codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPY));
    codeGen->FP_Mul();
    codeGen->FP_Sub();
    codeGen->FP_PullSingle(GetVectorElement(nFd, VECTOR_COMPZ));
}

void VUShared::RINIT(CCodeGen* codeGen, CMIPS* pCtx, uint8 nFs, uint8 nFsf)
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(pB);
	//{
	//	CCodeGen::PushVar(&pCtx->m_State.nCOP2[nFs].nV[nFsf]);
	//	CCodeGen::PushCst(0x007FFFFF);
	//	CCodeGen::And();
	//	CCodeGen::PullVar(&pCtx->m_State.nCOP2R);
	//}
	//CCodeGen::End();
}

void VUShared::RSQRT(CCodeGen* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf)
{
    codeGen->FP_PushSingle(GetVectorElement(nFs, nFsf));
    codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
    codeGen->FP_Rsqrt();
    codeGen->FP_Mul();
    codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP2Q));
}

void VUShared::RXOR(CCodeGen* codeGen, CMIPS* pCtx, uint8 nFs, uint8 nFsf)
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(pB);
	//{
	//	CCodeGen::PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[nFsf]));
	//	CCodeGen::PushRel(offsetof(CMIPS, m_State.nCOP2R));
	//	CCodeGen::Xor();
	//	CCodeGen::PushCst(0x007FFFFF);
	//	CCodeGen::And();
	//	CCodeGen::PullRel(offsetof(CMIPS, m_State.nCOP2R));
	//}
	//CCodeGen::End();
}

void VUShared::SQRT(CCodeGen* codeGen, uint8 nFt, uint8 nFtf)
{
    codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
    codeGen->FP_Sqrt();
    codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP2Q));
}

void VUShared::SUB(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_SubS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::SUBbc(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_SubS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::SUBi(CCodeGen* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_SubS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}
