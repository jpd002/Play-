#include "VUShared.h"
#include "MIPS.h"
#include "offsetof_def.h"

#define LATENCY_DIV     (7)
#define LATENCY_SQRT    (7)
#define LATENCY_RSQRT   (13)

const VUShared::PIPEINFO g_pipeInfoQ =
{
    offsetof(CMIPS, m_State.nCOP2Q),
//    offsetof(CMIPS, m_State.pipeQ.heldValue),
    offsetof(CMIPS, m_State.nCOP2Q),
    offsetof(CMIPS, m_State.pipeQ.target)
};

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

void VUShared::PullVector(CMipsJitter* codeGen, uint8 dest, size_t vector)
{
    codeGen->MD_PullRel(
        DestinationHasElement(dest, 0) ? vector + 0x0 : SIZE_MAX,
        DestinationHasElement(dest, 1) ? vector + 0x4 : SIZE_MAX,
        DestinationHasElement(dest, 2) ? vector + 0x8 : SIZE_MAX,
        DestinationHasElement(dest, 3) ? vector + 0xC : SIZE_MAX);
}

void VUShared::ADDA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand)
{
    codeGen->MD_PushRel(fs);
    if(expand)
    {
        codeGen->MD_PushRelExpand(ft);
    }
    else
    {
        codeGen->MD_PushRel(ft);
    }
    codeGen->MD_AddS();
    PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MADD_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(fs);
    if(expand)
    {
        codeGen->MD_PushRelExpand(ft);
    }
    else
    {
        codeGen->MD_PushRel(ft);
    }
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, dest, fd);
}

void VUShared::MSUB_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(fs);
    if(expand)
    {
        codeGen->MD_PushRelExpand(ft);
    }
    else
    {
        codeGen->MD_PushRel(ft);
    }
    codeGen->MD_MulS();
    codeGen->MD_SubS();
    PullVector(codeGen, dest, fd);
}

void VUShared::ABS(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_AbsS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::ADD(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
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

void VUShared::ADDbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::ADDi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::ADDq(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint32 address)
{
    VerifyPipeline(g_pipeInfoQ, codeGen, address);
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2Q));
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::ADDA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft)
{
    ADDA_base(codeGen, dest,
        offsetof(CMIPS, m_State.nCOP2[fs]),
        offsetof(CMIPS, m_State.nCOP2[ft]),
        false);
}

void VUShared::ADDAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc)
{
    ADDA_base(codeGen, dest,
        offsetof(CMIPS, m_State.nCOP2[fs]),
        offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
        true);
}

void VUShared::CLIP(CMipsJitter* codeGen, uint8 nFs, uint8 nFt)
{
	//Reimplement
	assert(0);

  //  //Create some space for the new test results
  //  codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
  //  codeGen->Shl(6);
  //  codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));

  //  for(unsigned int i = 0; i < 3; i++)
  //  {
  //      //c > +|w|
  //      codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[nFt].nV[3]));
  //      codeGen->FP_Abs();
  //      codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
		//codeGen->FP_Cmp(Jitter::CONDITION_AB);

  //      codeGen->BeginIf(true);
  //      {
  //          codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
  //          codeGen->PushCst(1 << ((i * 2) + 0));
  //          codeGen->Or();
  //          codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));
  //      }
  //      codeGen->EndIf();

  //      //c < -|w|
  //      codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[nFt].nV[3]));
  //      codeGen->FP_Abs();
  //      codeGen->FP_Neg();
  //      codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
  //      codeGen->FP_Cmp(Jitter::CONDITION_BL);

  //      codeGen->BeginIf(true);
  //      {
  //          codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
  //          codeGen->PushCst(1 << ((i * 2) + 1));
  //          codeGen->Or();
  //          codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));
  //      }
  //      codeGen->EndIf();
  //  }
}

void VUShared::DIV(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf, uint32 address, unsigned int pipeMult)
{
	//Reimplement
	assert(0);

    //size_t destination = g_pipeInfoQ.heldValue;
    //QueueInPipeline(g_pipeInfoQ, codeGen, address + (pipeMult * 4 * LATENCY_DIV));

    ////Check for zero
    //codeGen->PushRel(GetVectorElement(nFt, nFtf));
    //codeGen->PushCst(0x7FFFFFFF);
    //codeGen->And();
    //codeGen->PushCst(0);
    //codeGen->Cmp(Jitter::CONDITION_EQ);

    //codeGen->BeginIfElse(true);
    //{
    //    codeGen->PushCst(0x7F7FFFFF);
    //    codeGen->PushRel(GetVectorElement(nFs, nFsf));
    //    codeGen->PushRel(GetVectorElement(nFt, nFtf));
    //    codeGen->Xor();
    //    codeGen->PushCst(0x80000000);
    //    codeGen->And();
    //    codeGen->Or();
    //    codeGen->PullRel(destination);
    //}
    //codeGen->BeginIfElseAlt();
    //{
    //    codeGen->FP_PushSingle(GetVectorElement(nFs, nFsf));
    //    codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
    //    codeGen->FP_Div();
    //    codeGen->FP_PullSingle(destination);
    //}
    //codeGen->EndIf();
}

void VUShared::FTOI0(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_ToWordTruncate();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::FTOI4(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushCstExpand(16.0f);
    codeGen->MD_MulS();
    codeGen->MD_ToWordTruncate();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::FTOI12(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushCstExpand(4096.0f);
    codeGen->MD_MulS();
    codeGen->MD_ToWordTruncate();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::ITOF0(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_ToSingle();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::ITOF4(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 fs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
    codeGen->MD_ToSingle();
    codeGen->MD_PushCstExpand(16.0f);
    codeGen->MD_DivS();
    PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[ft]));
}

void VUShared::ITOF12(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 fs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
    codeGen->MD_ToSingle();
    codeGen->MD_PushCstExpand(4096.0f);
    codeGen->MD_DivS();
    PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[ft]));
}

void VUShared::ITOF15(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 fs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
    codeGen->MD_ToSingle();
    codeGen->MD_PushCstExpand(32768.0f);
    codeGen->MD_DivS();
    PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[ft]));
}

void VUShared::MADD(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft)
{
    MADD_base(codeGen, dest,
        offsetof(CMIPS, m_State.nCOP2[fd]),
        offsetof(CMIPS, m_State.nCOP2[fs]),
        offsetof(CMIPS, m_State.nCOP2[ft]),
        false);
}

void VUShared::MADDbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc)
{
    MADD_base(codeGen, dest,
        offsetof(CMIPS, m_State.nCOP2[fd]),
        offsetof(CMIPS, m_State.nCOP2[fs]),
        offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
        true);
}

void VUShared::MADDq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 address)
{
    MADD_base(codeGen, dest,
        offsetof(CMIPS, m_State.nCOP2[fd]),
        offsetof(CMIPS, m_State.nCOP2[fs]),
        offsetof(CMIPS, m_State.nCOP2Q),
        true);
}

void VUShared::MADDA(CMipsJitter* codeGen, uint8 nDest, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MADDAbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MADDAi(CMipsJitter* codeGen, uint8 nDest, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    codeGen->MD_AddS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MAX(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MaxS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MAXbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MaxS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MINI(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MinS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MINIbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MinS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MINIi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MinS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MOVE(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    for(unsigned int i = 0; i < 4; i++)
    {
	    if(!DestinationHasElement(nDest, i)) continue;

	    codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
	    codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[nFt].nV[i]));
    }
}

void VUShared::MR32(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
    size_t offset[4];

    if(nFs == nFt)
    {
        offset[0] = offsetof(CMIPS, m_State.nCOP2[nFs].nV[1]);
        offset[1] = offsetof(CMIPS, m_State.nCOP2[nFs].nV[2]);
        offset[2] = offsetof(CMIPS, m_State.nCOP2[nFs].nV[3]);
        offset[3] = offsetof(CMIPS, m_State.nCOP2T);

        codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[0]));
        codeGen->PullRel(offset[3]);
    }
    else
    {
        offset[0] = offsetof(CMIPS, m_State.nCOP2[nFs].nV[1]);
        offset[1] = offsetof(CMIPS, m_State.nCOP2[nFs].nV[2]);
        offset[2] = offsetof(CMIPS, m_State.nCOP2[nFs].nV[3]);
        offset[3] = offsetof(CMIPS, m_State.nCOP2[nFs].nV[0]);
    }

    for(unsigned int i = 0; i < 4; i++)
    {
        if(!DestinationHasElement(nDest, i)) continue;
        codeGen->PushRel(offset[i]);
        codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[nFt].nV[i]));
    }
}

void VUShared::MSUBbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc)
{
    MSUB_base(codeGen, dest,
        offsetof(CMIPS, m_State.nCOP2[fd]),
        offsetof(CMIPS, m_State.nCOP2[fs]),
        offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
        true);
}

void VUShared::MSUBi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    MSUB_base(codeGen, nDest,
        offsetof(CMIPS, m_State.nCOP2[nFd]),
        offsetof(CMIPS, m_State.nCOP2[nFs]),
        offsetof(CMIPS, m_State.nCOP2I),
        true);
}

void VUShared::MSUBAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]));
    codeGen->MD_MulS();
    codeGen->MD_SubS();
    PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MSUBAi(CMipsJitter* codeGen, uint8 nDest, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    codeGen->MD_SubS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MUL(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULq(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint32 address)
{
    VerifyPipeline(g_pipeInfoQ, codeGen, address);
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2Q));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULA(CMipsJitter* codeGen, uint8 nDest, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MULAbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::MULAi(CMipsJitter* codeGen, uint8 nDest, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_MulS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::OPMULA(CMipsJitter* codeGen, uint8 nFs, uint8 nFt)
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

void VUShared::OPMSUB(CMipsJitter* codeGen, uint8 nFd, uint8 nFs, uint8 nFt)
{
	//Reimplement
	assert(0);

	//if(nFd == 0) 
	//{
	//	//Atelier Iris - OPMSUB with VF0 as FD...
	//	//This is probably to set a flag which is tested a bit further
	//	//The flag tested is Sz
 //       codeGen->PushCst(0);

	//	codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPZ));
	//	codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPX));
	//	codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPY));
	//	codeGen->FP_Mul();
	//	codeGen->FP_Sub();

	//	codeGen->FP_Cmp(Jitter::CONDITION_BL);
	//	
	//	codeGen->BeginIfElse(true);
	//	{
	//		codeGen->PushCst(0xFFFFFFFF);
	//		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2SF.nV[2]));
	//	}
	//	codeGen->BeginIfElseAlt();
	//	{
	//		codeGen->PushCst(0);
	//		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2SF.nV[2]));
	//	}
	//	codeGen->EndIf();
	//	return;
	//}

 //   //X
 //   codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPX));
 //   codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPY));
 //   codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPZ));
 //   codeGen->FP_Mul();
 //   codeGen->FP_Sub();
 //   codeGen->FP_PullSingle(GetVectorElement(nFd, VECTOR_COMPX));

 //   //Y
 //   codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPY));
 //   codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPZ));
 //   codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPX));
 //   codeGen->FP_Mul();
 //   codeGen->FP_Sub();
 //   codeGen->FP_PullSingle(GetVectorElement(nFd, VECTOR_COMPY));

 //   //Z
 //   codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPZ));
 //   codeGen->FP_PushSingle(GetVectorElement(nFs, VECTOR_COMPX));
 //   codeGen->FP_PushSingle(GetVectorElement(nFt, VECTOR_COMPY));
 //   codeGen->FP_Mul();
 //   codeGen->FP_Sub();
 //   codeGen->FP_PullSingle(GetVectorElement(nFd, VECTOR_COMPZ));
}

void VUShared::RINIT(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf)
{
    codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[nFsf]));
    codeGen->PushCst(0x007FFFFF);
    codeGen->And();
    codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2R));
}

void VUShared::RGET(CMipsJitter* codeGen, uint8 dest, uint8 ft)
{
    for(unsigned int i = 0; i < 4; i++)
    {
        if(!VUShared::DestinationHasElement(dest, i)) continue;

        codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2R));
        codeGen->PushCst(0x3F800000);
        codeGen->Or();
        codeGen->PullRel(VUShared::GetVectorElement(ft, i));
    }
}

void VUShared::RNEXT(CMipsJitter* codeGen, uint8 dest, uint8 ft)
{
    //Compute next R
    codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2R));
    codeGen->PushCst(0xDEADBEEF);
    codeGen->Xor();
    codeGen->PushCst(0xDEADBEEF);
    codeGen->Add();
    codeGen->PushCst(0x007FFFFF);
    codeGen->And();
    codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2R));

    RGET(codeGen, dest, ft);
}

void VUShared::RSQRT(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf, uint32 address, unsigned int pipeMult)
{
    size_t destination = g_pipeInfoQ.heldValue;
    QueueInPipeline(g_pipeInfoQ, codeGen, address + (pipeMult * 4 * LATENCY_RSQRT));

    codeGen->FP_PushSingle(GetVectorElement(nFs, nFsf));
    codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
    codeGen->FP_Rsqrt();
    codeGen->FP_Mul();
    codeGen->FP_PullSingle(destination);
}

void VUShared::RXOR(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf)
{
    codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[nFsf]));
    codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2R));
    codeGen->Xor();
    codeGen->PushCst(0x007FFFFF);
    codeGen->And();
    codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2R));
}

void VUShared::SQRT(CMipsJitter* codeGen, uint8 nFt, uint8 nFtf, uint32 address, unsigned int pipeMult)
{
    size_t destination = g_pipeInfoQ.heldValue;
    QueueInPipeline(g_pipeInfoQ, codeGen, address + (pipeMult * 4 * LATENCY_SQRT));

    codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
    codeGen->FP_Sqrt();
    codeGen->FP_PullSingle(destination);
}

void VUShared::SUB(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
    codeGen->MD_SubS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::SUBbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
    codeGen->MD_SubS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::SUBi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
    codeGen->MD_SubS();
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::SUBAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc)
{
    codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
    codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]));
    codeGen->MD_SubS();
    PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A));
}

void VUShared::WAITQ(CMipsJitter* codeGen)
{
    FlushPipeline(g_pipeInfoQ, codeGen);
}

void VUShared::FlushPipeline(const PIPEINFO& pipeInfo, CMipsJitter* codeGen)
{
	//Reimplement
	assert(0);

    //return;

    ////Dump the current value if one pending
    //codeGen->PushCst(MIPS_INVALID_PC);
    //codeGen->PushRel(pipeInfo.target);
    //codeGen->Cmp(Jitter::CONDITION_EQ);

    //codeGen->BeginIf(false);
    //{
    //    codeGen->PushRel(pipeInfo.heldValue);
    //    codeGen->PullRel(pipeInfo.value);

    //    codeGen->PushCst(MIPS_INVALID_PC);
    //    codeGen->PullRel(pipeInfo.target);
    //}
    //codeGen->EndIf();
}

void VUShared::QueueInPipeline(const PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 targetAddress)
{
    return;

    FlushPipeline(pipeInfo, codeGen);

    //Set target
    codeGen->PushCst(targetAddress);
    codeGen->PullRel(pipeInfo.target);
}

void VUShared::VerifyPipeline(const PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 currentAddress)
{
	//Reimplement
	assert(0);
    //return;

    ////Dump current value if it's ready
    //codeGen->PushCst(currentAddress);
    //codeGen->PushRel(pipeInfo.target);
    //codeGen->Cmp(Jitter::CONDITION_BL);

    //codeGen->BeginIf(false);
    //{
    //    codeGen->PushRel(pipeInfo.heldValue);
    //    codeGen->PullRel(pipeInfo.value);

    //    codeGen->PushCst(MIPS_INVALID_PC);
    //    codeGen->PullRel(pipeInfo.target);
    //}
    //codeGen->EndIf();
}
