#include "VUShared.h"
#include "MIPS.h"
#include "offsetof_def.h"
#include "FpMulTruncate.h"
#include "MemoryUtils.h"

#define LATENCY_DIV     (7 - 1)
#define LATENCY_SQRT    (7 - 1)
#define LATENCY_RSQRT   (13 - 1)

const VUShared::PIPEINFO VUShared::g_pipeInfoQ =
{
    offsetof(CMIPS, m_State.nCOP2Q),
    offsetof(CMIPS, m_State.pipeQ.heldValue),
    offsetof(CMIPS, m_State.pipeQ.counter)
};

using namespace VUShared;

bool VUShared::DestinationHasElement(uint8 nDest, unsigned int nElement)
{
	return (nDest & (1 << (nElement ^ 0x03))) != 0;
}

void VUShared::ComputeMemAccessAddr(CMipsJitter* codeGen, unsigned int baseRegister, uint32 baseOffset, uint32 destOffset)
{
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[baseRegister]));
	if(baseOffset != 0)
	{
		codeGen->PushCst(baseOffset);
		codeGen->Add();
	}
	codeGen->Shl(4);

	if(destOffset != 0)
	{
		codeGen->PushCst(destOffset);
		codeGen->Add();
	}

	//Mask address
	codeGen->PushCst(0x3FFF);
	codeGen->And();
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
	assert(vector != offsetof(CMIPS, m_State.nCOP2[0]));
	codeGen->MD_PullRel(vector,
		DestinationHasElement(dest, 0),
		DestinationHasElement(dest, 1),
		DestinationHasElement(dest, 2),
		DestinationHasElement(dest, 3));
}

void VUShared::PushIntegerRegister(CMipsJitter* codeGen, unsigned int nRegister)
{
	if(nRegister == 0)
	{
		codeGen->PushCst(0);
	}
	else
	{
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[nRegister]));
	}
}

void VUShared::TestSZFlags(CMipsJitter* codeGen, uint8 dest, uint8 reg)
{
	//--- Generate address
	const int macOpLatency = 3;

	codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeMac.slots));

	codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.counter));
	codeGen->PushCst(macOpLatency);
	codeGen->Add();
	codeGen->PushCst(MACFLAG_PIPELINE_SLOTS - 1);
	codeGen->And();

	//Multiply offset by 4
	codeGen->Shl(2);

	codeGen->AddRef();

	//--- Generate value
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[reg]));
	codeGen->MD_IsNegative();
	codeGen->Shl(4);

	//Not even used anywhere...
//	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[reg]));
//	codeGen->MD_IsZero();
//	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2ZF));

//	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2MF));

	codeGen->PushCst(0x80000000);
	codeGen->Or();

	//--- Store value
	codeGen->StoreAtRef();
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

void VUShared::MSUBA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand)
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
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A));
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
    PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));

	TestSZFlags(codeGen, nDest, nFd);
}

void VUShared::ADDbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc)
{
	if(nFd == 0)
	{
		//Use the temporary register to store the result
		nFd = 32;
	}

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

void VUShared::ADDq(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
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
	//Create some space for the new test results
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
	codeGen->Shl(6);
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));

	for(unsigned int i = 0; i < 3; i++)
	{
		//c > +|w|
		codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
		codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[nFt].nV[3]));
		codeGen->FP_Abs();

		codeGen->FP_Cmp(Jitter::CONDITION_AB);
		codeGen->PushCst(0);
		codeGen->BeginIf(Jitter::CONDITION_NE);
		{
			codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
			codeGen->PushCst(1 << ((i * 2) + 0));
			codeGen->Or();
			codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));
		}
		codeGen->EndIf();

		//c < -|w|
		codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
		codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[nFt].nV[3]));
		codeGen->FP_Abs();
		codeGen->FP_Neg();

		codeGen->FP_Cmp(Jitter::CONDITION_BL);
		codeGen->PushCst(0);
		codeGen->BeginIf(Jitter::CONDITION_NE);
		{
			codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
			codeGen->PushCst(1 << ((i * 2) + 1));
			codeGen->Or();
			codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));
		}
		codeGen->EndIf();
	}
}

void VUShared::DIV(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf, uint32 address, unsigned int pipeMult)
{
	size_t destination = g_pipeInfoQ.heldValue;
	QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_DIV);

	//Check for zero
	codeGen->PushRel(GetVectorElement(nFt, nFtf));
	codeGen->PushCst(0x7FFFFFFF);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_EQ);
	{
		codeGen->PushCst(0x7F7FFFFF);
		codeGen->PushRel(GetVectorElement(nFs, nFsf));
		codeGen->PushRel(GetVectorElement(nFt, nFtf));
		codeGen->Xor();
		codeGen->PushCst(0x80000000);
		codeGen->And();
		codeGen->Or();
		codeGen->PullRel(destination);
	}
	codeGen->Else();
	{
		codeGen->FP_PushSingle(GetVectorElement(nFs, nFsf));
		codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
		codeGen->FP_Div();
		codeGen->FP_PullSingle(destination);
	}
	codeGen->EndIf();
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

void VUShared::IADDI(CMipsJitter* codeGen, uint8 it, uint8 is, uint8 imm5)
{
	PushIntegerRegister(codeGen, is);
	codeGen->PushCst(imm5 | ((imm5 & 0x10) != 0 ? 0xFFFFFFE0 : 0x0));
	codeGen->Add();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
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
	if(fd == 0)
	{
		//Use the temporary register to store the result
		fd = 32;
	}

    MADD_base(codeGen, dest,
        offsetof(CMIPS, m_State.nCOP2[fd]),
        offsetof(CMIPS, m_State.nCOP2[fs]),
        offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
        true);
}

void VUShared::MADDi(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs)
{
    MADD_base(codeGen, dest,
        offsetof(CMIPS, m_State.nCOP2[fd]),
        offsetof(CMIPS, m_State.nCOP2[fs]),
        offsetof(CMIPS, m_State.nCOP2I),
        true);
}

void VUShared::MADDq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs)
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

void VUShared::MSUB(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft)
{
	MSUB_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fd]),
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft]),
		false);
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

void VUShared::MSUBq(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
    MSUB_base(codeGen, nDest,
        offsetof(CMIPS, m_State.nCOP2[nFd]),
        offsetof(CMIPS, m_State.nCOP2[nFs]),
        offsetof(CMIPS, m_State.nCOP2Q),
        true);
}

void VUShared::MSUBA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft)
{
	MSUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft]),
		false);
}

void VUShared::MSUBAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc)
{
	MSUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
		true);
}

void VUShared::MSUBAi(CMipsJitter* codeGen, uint8 dest, uint8 fs)
{
	MSUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2I),
		true);
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

	TestSZFlags(codeGen, nDest, nFd);
}

void VUShared::MULi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
	for(unsigned int i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(nDest, i)) continue;

		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2I));
		codeGen->Call(&FpMulTruncate, 2, true);
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[nFd].nV[i]));
	}
}

void VUShared::MULq(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint32 address)
{
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

void VUShared::MULAq(CMipsJitter* codeGen, uint8 nDest, uint8 nFs)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2Q));
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
	if(nFd == 0)
	{
		//This is done by many games
		//Use the temporary register to store the result
		nFd = 32;
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

	TestSZFlags(codeGen, 0xF, nFd);
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
    QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_RSQRT);

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

void VUShared::SQI(CMipsJitter* codeGen, uint8 dest, uint8 is, uint8 it, uint32 baseAddress)
{
	ComputeMemAccessAddr(codeGen, it, 0, 0);
	if(baseAddress != 0)
	{
		codeGen->PushCst(baseAddress);
		codeGen->Add();
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		if(VUShared::DestinationHasElement(dest, i))
		{
			codeGen->PushCtx();
			codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[is].nV[i]));
			codeGen->PushIdx(2);
			codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);
		}

		if(i != 3)
		{
			codeGen->PushCst(4);
			codeGen->Add();
		}
	}

	codeGen->PullTop();

	//Increment
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
	codeGen->PushCst(1);
	codeGen->Add();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
}

void VUShared::SQRT(CMipsJitter* codeGen, uint8 nFt, uint8 nFtf)
{
    size_t destination = g_pipeInfoQ.heldValue;
    QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_SQRT);

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
	if(nFd == 0)
	{
		//Use the temporary register to store the result
		nFd = 32;
	}

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

	TestSZFlags(codeGen, nDest, nFd);
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
	codeGen->PushCst(0);
	codeGen->PullRel(pipeInfo.counter);

	codeGen->PushRel(pipeInfo.heldValue);
	codeGen->PullRel(pipeInfo.value);
}

void VUShared::QueueInPipeline(const PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 latency)
{
    //Set target
    codeGen->PushCst(latency);
	codeGen->Shl(8);
    codeGen->PullRel(pipeInfo.counter);
}

void VUShared::AdvancePipeline(const PIPEINFO& pipeInfo, CMipsJitter* codeGen)
{
	//Check if pipeline is ready
	codeGen->PushCst(0);
	codeGen->PushRel(pipeInfo.counter);
	codeGen->Srl(8);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		codeGen->PushRel(pipeInfo.counter);
		codeGen->Srl(8);
		codeGen->PushCst(-1);
		codeGen->Add();

		codeGen->PushTop();
		codeGen->PushCst(0);
		codeGen->Cmp(Jitter::CONDITION_EQ);

		codeGen->Swap();
		codeGen->Shl(8);

		codeGen->Or();
		codeGen->PullRel(pipeInfo.counter);
	}
	codeGen->EndIf();

	codeGen->PushCst(0xFF);
	codeGen->PushRel(pipeInfo.counter);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		FlushPipeline(pipeInfo, codeGen);
	}
	codeGen->EndIf();
}

void VUShared::AdvanceMacFlagPipeline(CMipsJitter* codeGen)
{
	//--- Load current value
	codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeMac.slots));
	codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.counter));
	codeGen->Shl(2);
	codeGen->AddRef();
	codeGen->LoadFromRef();

	//--- Test if high order bit is set
	codeGen->PushCst(0x80000000);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		//--- Load current value (once again)
		codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeMac.slots));
		codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.counter));
		codeGen->Shl(2);
		codeGen->AddRef();
		codeGen->PushTop();

		//--- Dump value in official flags register
		codeGen->LoadFromRef();
		codeGen->PushCst(~0x80000000);
		codeGen->And();
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2MF));

		//--- Clear value
		codeGen->PushCst(0);
		codeGen->StoreAtRef();
	}
	codeGen->EndIf();

	//--- Increment counter
	codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.counter));
	codeGen->PushCst(1);
	codeGen->Add();

	codeGen->PushCst(MACFLAG_PIPELINE_SLOTS - 1);
	codeGen->And();
	
	codeGen->PullRel(offsetof(CMIPS, m_State.pipeMac.counter));
}
