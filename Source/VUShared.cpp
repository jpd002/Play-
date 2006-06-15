#include "VUShared.h"
#include "MIPS.h"
#include "CodeGen_FPU.h"
#include "CodeGen_VUF128.h"

using namespace VUShared;
using namespace CodeGen;

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

void VUShared::PullVector(uint8 nDest, uint128* pVector)
{
	CVUF128::Pull( \
		DestinationHasElement(nDest, 0) ? &pVector->nV0 : NULL, \
		DestinationHasElement(nDest, 1) ? &pVector->nV1 : NULL, \
		DestinationHasElement(nDest, 2) ? &pVector->nV2 : NULL, \
		DestinationHasElement(nDest, 3) ? &pVector->nV3 : NULL);
}

void VUShared::ABS(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFt, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		unsigned int i;

		for(i = 0; i < 4; i++)
		{
			if(!DestinationHasElement(nDest, i)) continue;

			CFPU::PushSingle(GetVectorElement(pCtx, nFs, i));
			CFPU::Abs();
			CFPU::PullSingle(GetVectorElement(pCtx, nFt, i));
		}
	}
	CCodeGen::End();
}

void VUShared::ADD(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt]);
		CVUF128::Add();

		//Get result flags
		CVUF128::PushTop();
		CVUF128::PushTop();

		CVUF128::IsNegative();
		PullVector(nDest, &pCtx->m_State.nCOP2SF);

		CVUF128::IsZero();
		PullVector(nDest, &pCtx->m_State.nCOP2ZF);

		//Save result
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::ADDbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::ADDi(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2I);
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::ADDq(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2Q);
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::ADDAbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2A);
	}
	CCodeGen::End();
}

void VUShared::CLIP(CCacheBlock* pB, CMIPS* pCtx, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		//Create some space for the new test results
		CCodeGen::PushVar(&pCtx->m_State.nCOP2CF);
		CCodeGen::Shl(6);
		CCodeGen::PullVar(&pCtx->m_State.nCOP2CF);

		for(unsigned int i = 0; i < 3; i++)
		{
			//c > +|w|
			CFPU::PushSingle(&pCtx->m_State.nCOP2[nFt].nV[3]);
			CFPU::Abs();
			CFPU::PushSingle(&pCtx->m_State.nCOP2[nFs].nV[i]);
			CFPU::Cmp(CCodeGen::CONDITION_AB);
			
			CCodeGen::BeginIf(true);
			{
				CCodeGen::PushVar(&pCtx->m_State.nCOP2CF);
				CCodeGen::PushCst(1 << ((i * 2) + 0));
				CCodeGen::Or();
				CCodeGen::PullVar(&pCtx->m_State.nCOP2CF);
			}
			CCodeGen::EndIf();

			//c < -|w|
			CFPU::PushSingle(&pCtx->m_State.nCOP2[nFt].nV[3]);
			CFPU::Abs();
			CFPU::Neg();
			CFPU::PushSingle(&pCtx->m_State.nCOP2[nFs].nV[i]);
			CFPU::Cmp(CCodeGen::CONDITION_BL);

			CCodeGen::BeginIf(true);
			{
				CCodeGen::PushVar(&pCtx->m_State.nCOP2CF);
				CCodeGen::PushCst(1 << ((i * 2) + 1));
				CCodeGen::Or();
				CCodeGen::PullVar(&pCtx->m_State.nCOP2CF);
			}
			CCodeGen::EndIf();
		}

	}
	CCodeGen::End();
}

void VUShared::DIV(CCacheBlock* pB, CMIPS* pCtx, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf)
{
	CCodeGen::Begin(pB);
	{
		//Check for zero
		CCodeGen::PushVar(GetVectorElement(pCtx, nFt, nFtf));
		CCodeGen::PushCst(0x7FFFFFFF);
		CCodeGen::And();
		CCodeGen::PushCst(0);
		CCodeGen::Cmp(CCodeGen::CONDITION_EQ);

		CCodeGen::BeginIfElse(true);
		{
			CCodeGen::PushCst(0x7F7FFFFF);
			CCodeGen::PushVar(GetVectorElement(pCtx, nFs, nFsf));
			CCodeGen::PushVar(GetVectorElement(pCtx, nFt, nFtf));
			CCodeGen::Xor();
			CCodeGen::PushCst(0x80000000);
			CCodeGen::And();
			CCodeGen::Or();
			CCodeGen::PullVar(&pCtx->m_State.nCOP2Q);
		}
		CCodeGen::BeginIfElseAlt();
		{
			CFPU::PushSingle(GetVectorElement(pCtx, nFs, nFsf));
			CFPU::PushSingle(GetVectorElement(pCtx, nFt, nFtf));
			CFPU::Div();
			CFPU::PullSingle(&pCtx->m_State.nCOP2Q);
		}
		CCodeGen::EndIf();
	}
	CCodeGen::End();
}

void VUShared::FTOI0(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFt, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::ToWordTruncate();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFt]);
	}
	CCodeGen::End();
}

void VUShared::FTOI4(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFt, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		//Get the fractional part
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Truncate();
		CVUF128::Sub();

		CVUF128::PushImm(16.0f);
		CVUF128::Mul();

		//Get the whole part
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Truncate();
		CVUF128::PushImm(16.0f);
		CVUF128::Mul();

		//Add both parts
		CVUF128::Add();

		//Save result
		CVUF128::ToWordTruncate();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFt]);
	}
	CCodeGen::End();
}

void VUShared::ITOF0(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFt, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::ToSingle();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFt]);
	}
	CCodeGen::End();
}

void VUShared::MADD(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2A);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt]);
		CVUF128::Mul();
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MADDbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2A);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Mul();
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MADDA(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2A);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt]);
		CVUF128::Mul();
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2A);
	}
	CCodeGen::End();
}

void VUShared::MADDAbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2A);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Mul();
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2A);
	}
	CCodeGen::End();
}

void VUShared::MADDAi(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2A);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2I);
		CVUF128::Mul();
		CVUF128::Add();
		PullVector(nDest, &pCtx->m_State.nCOP2A);
	}
	CCodeGen::End();
}

void VUShared::MAX(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt]);
		CVUF128::Max();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MAXbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Max();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MINI(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt]);
		CVUF128::Min();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MINIbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Min();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MINIi(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2I);
		CVUF128::Min();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MOVE(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFt, uint8 nFs)
{
	unsigned int i;

	CCodeGen::Begin(pB);
	{
		for(i = 0; i < 4; i++)
		{
			if(!DestinationHasElement(nDest, i)) continue;

			CCodeGen::PushVar(&pCtx->m_State.nCOP2[nFs].nV[i]);
			CCodeGen::PullVar(&pCtx->m_State.nCOP2[nFt].nV[i]);
		}
	}
	CCodeGen::End();
}

void VUShared::MR32(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFt, uint8 nFs)
{
	unsigned int i;

	CCodeGen::Begin(pB);
	{
		for(i = 0; i < 4; i++)
		{
			if(!DestinationHasElement(nDest, i)) continue;

			CCodeGen::PushVar(&pCtx->m_State.nCOP2[nFs].nV[(i + 1) & 3]);
			CCodeGen::PullVar(&pCtx->m_State.nCOP2[nFt].nV[i]);
		}
	}
	CCodeGen::End();
}

void VUShared::MSUBi(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2A);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2I);
		CVUF128::Mul();
		CVUF128::Sub();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MSUBAi(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2A);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2I);
		CVUF128::Mul();
		CVUF128::Sub();
		PullVector(nDest, &pCtx->m_State.nCOP2A);
	}
	CCodeGen::End();
}

void VUShared::MUL(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt]);
		CVUF128::Mul();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MULbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Mul();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MULi(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2I);
		CVUF128::Mul();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MULq(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2Q);
		CVUF128::Mul();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::MULA(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt]);
		CVUF128::Mul();
		PullVector(nDest, &pCtx->m_State.nCOP2A);
	}
	CCodeGen::End();
}

void VUShared::MULAbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Mul();
		PullVector(nDest, &pCtx->m_State.nCOP2A);
	}
	CCodeGen::End();
}

void VUShared::MULAi(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2I);
		CVUF128::Mul();
		PullVector(nDest, &pCtx->m_State.nCOP2A);
	}
	CCodeGen::End();
}

void VUShared::OPMULA(CCacheBlock* pB, CMIPS* pCtx, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		//ACCx
		CFPU::PushSingle(GetVectorElement(pCtx, nFs, VECTOR_COMPY));
		CFPU::PushSingle(GetVectorElement(pCtx, nFt, VECTOR_COMPZ));
		CFPU::Mul();
		CFPU::PullSingle(GetAccumulatorElement(pCtx, VECTOR_COMPX));

		//ACCy
		CFPU::PushSingle(GetVectorElement(pCtx, nFs, VECTOR_COMPZ));
		CFPU::PushSingle(GetVectorElement(pCtx, nFt, VECTOR_COMPX));
		CFPU::Mul();
		CFPU::PullSingle(GetAccumulatorElement(pCtx, VECTOR_COMPY));

		//ACCz
		CFPU::PushSingle(GetVectorElement(pCtx, nFs, VECTOR_COMPX));
		CFPU::PushSingle(GetVectorElement(pCtx, nFt, VECTOR_COMPY));
		CFPU::Mul();
		CFPU::PullSingle(GetAccumulatorElement(pCtx, VECTOR_COMPZ));
	}
	CCodeGen::End();
}

void VUShared::OPMSUB(CCacheBlock* pB, CMIPS* pCtx, uint8 nFd, uint8 nFs, uint8 nFt)
{
	if(nFd == 0) 
	{
		//Atelier Iris - OPMSUB with VF0 as FD...
		//This is probably to set a flag which is tested a bit further
		//The flag tested is Sz
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
	}

	CCodeGen::Begin(pB);
	{
		//X
		CFPU::PushSingle(GetAccumulatorElement(pCtx, VECTOR_COMPX));
		CFPU::PushSingle(GetVectorElement(pCtx, nFs, VECTOR_COMPY));
		CFPU::PushSingle(GetVectorElement(pCtx, nFt, VECTOR_COMPZ));
		CFPU::Mul();
		CFPU::Sub();
		CFPU::PullSingle(GetVectorElement(pCtx, nFd, VECTOR_COMPX));

		//Y
		CFPU::PushSingle(GetAccumulatorElement(pCtx, VECTOR_COMPY));
		CFPU::PushSingle(GetVectorElement(pCtx, nFs, VECTOR_COMPZ));
		CFPU::PushSingle(GetVectorElement(pCtx, nFt, VECTOR_COMPX));
		CFPU::Mul();
		CFPU::Sub();
		CFPU::PullSingle(GetVectorElement(pCtx, nFd, VECTOR_COMPY));

		//Z
		CFPU::PushSingle(GetAccumulatorElement(pCtx, VECTOR_COMPZ));
		CFPU::PushSingle(GetVectorElement(pCtx, nFs, VECTOR_COMPX));
		CFPU::PushSingle(GetVectorElement(pCtx, nFt, VECTOR_COMPY));
		CFPU::Mul();
		CFPU::Sub();
		CFPU::PullSingle(GetVectorElement(pCtx, nFd, VECTOR_COMPZ));
	}
	CCodeGen::End();
}

void VUShared::RSQRT(CCacheBlock* pB, CMIPS* pCtx, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf)
{
	CCodeGen::Begin(pB);
	{
		CFPU::PushSingle(GetVectorElement(pCtx, nFs, nFsf));
		CFPU::PushSingle(GetVectorElement(pCtx, nFt, nFtf));
		CFPU::Sqrt();
		CFPU::Div();
		CFPU::PullSingle(&pCtx->m_State.nCOP2Q);
	}
	CCodeGen::End();
}

void VUShared::SQRT(CCacheBlock* pB, CMIPS* pCtx, uint8 nFt, uint8 nFtf)
{
	CCodeGen::Begin(pB);
	{
		CFPU::PushSingle(GetVectorElement(pCtx, nFt, nFtf));
		CFPU::Sqrt();
		CFPU::PullSingle(&pCtx->m_State.nCOP2Q);
	}
	CCodeGen::End();
}

void VUShared::SUB(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt]);
		CVUF128::Sub();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::SUBbc(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2[nFt].nV[nBc]);
		CVUF128::Sub();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}

void VUShared::SUBi(CCacheBlock* pB, CMIPS* pCtx, uint8 nDest, uint8 nFd, uint8 nFs)
{
	CCodeGen::Begin(pB);
	{
		CVUF128::Push(&pCtx->m_State.nCOP2[nFs]);
		CVUF128::Push(&pCtx->m_State.nCOP2I);
		CVUF128::Sub();
		PullVector(nDest, &pCtx->m_State.nCOP2[nFd]);
	}
	CCodeGen::End();
}
