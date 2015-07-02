#include "VUShared.h"
#include "../MIPS.h"
#include "../MemoryUtils.h"
#include "offsetof_def.h"
#include "FpMulTruncate.h"
#include "FpAddTruncate.h"

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

uint32 VUShared::GetDestOffset(uint8 dest)
{
	if(dest & 0x0001) return 0xC;
	if(dest & 0x0002) return 0x8;
	if(dest & 0x0004) return 0x4;
	if(dest & 0x0008) return 0x0;

	return 0;
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

void VUShared::ClampVector(CMipsJitter* codeGen)
{
	//This will transform any NaN/INF (exponent == 0xFF) into a number with exponent == 0xFE
	//and will leave all other numbers intact
	static const uint32 exponentMask = 0x7F800000;
	codeGen->PushTop();
	codeGen->MD_PushCstExpand(exponentMask);
	codeGen->MD_And();
	codeGen->MD_PushCstExpand(exponentMask);
	codeGen->MD_CmpEqW();
	codeGen->MD_SrlW(31);
	codeGen->MD_SllW(23);
	codeGen->MD_Not();
	codeGen->MD_And();
}

void VUShared::TestSZFlags(CMipsJitter* codeGen, uint8 dest, size_t regOffset, uint32 relativePipeTime)
{
	const int macOpLatency = 4;

	//Write value time
	{
		//Generate value time address
		codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeMac.pipeTimes));

		//Get offset and multiply by 4
		codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.index));
		codeGen->Shl(2);
		codeGen->AddRef();

		//Generate value time
		codeGen->PushRel(offsetof(CMIPS, m_State.pipeTime));
		codeGen->PushCst(relativePipeTime + macOpLatency);
		codeGen->Add();

		//--- Store value
		codeGen->StoreAtRef();
	}

	//Write value
	{
		//Generate value time address
		codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeMac.values));

		//Get offset and multiply by 4
		codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.index));
		codeGen->Shl(2);
		codeGen->AddRef();

		//--- S flag
		codeGen->MD_PushRel(regOffset);
		codeGen->MD_IsNegative();
		codeGen->Shl(4);

		//--- Z flag
		codeGen->MD_PushRel(regOffset);
		codeGen->MD_IsZero();
		codeGen->Or();

		//Clear flags of inactive FMAC units
		codeGen->PushCst((dest << 4) | dest);
		codeGen->And();

		//Update sticky flags
		codeGen->PushTop();
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2SF));
		codeGen->Or();
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2SF));

		//--- Store value
		codeGen->StoreAtRef();
	}

	//Increment counter
	codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.index));
	codeGen->PushCst(1);
	codeGen->Add();
	codeGen->PushCst(MACFLAG_PIPELINE_SLOTS - 1);
	codeGen->And();
	codeGen->PullRel(offsetof(CMIPS, m_State.pipeMac.index));
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

void VUShared::MADD_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand, uint32 relativePipeTime)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
	codeGen->MD_PushRel(fs);
	//Clamping is needed by Baldur's Gate Deadly Alliance here because it multiplies junk values (potentially NaN/INF) by 0
	ClampVector(codeGen);
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
	TestSZFlags(codeGen, dest, fd, relativePipeTime);
}

void VUShared::MADDA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand, uint32 relativePipeTime)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
	codeGen->MD_PushRel(fs);
	//Clamping is needed by Dynasty Warriors 2 here because it multiplies junk values (potentially NaN/INF) by some other value
	ClampVector(codeGen);
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
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A));
	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A), relativePipeTime);
}

void VUShared::SUBA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand)
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
	codeGen->MD_SubS();
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A));
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

void VUShared::MSUBA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand, uint32 relativePipeTime)
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
	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A), relativePipeTime);
}

void VUShared::ABS(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_AbsS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::ADD(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint32 relativePipeTime)
{
	if(nFd == 0)
	{
		//Use the temporary register to store the result
		nFd = 32;
	}

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
	codeGen->MD_AddS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));

	TestSZFlags(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]), relativePipeTime);
}

void VUShared::ADDbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc, uint32 relativePipeTime)
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

	TestSZFlags(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]), relativePipeTime);
}

void VUShared::ADDi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint32 relativePipeTime)
{
	if(nFd == 0)
	{
		//Use the temporary register to store the result
		nFd = 32;
	}

#if 1
	for(unsigned int i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(nDest, i)) continue;

		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2I));
		codeGen->Call(reinterpret_cast<void*>(&FpAddTruncate), 2, true);
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[nFd].nV[i]));
	}
#else
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
	codeGen->MD_AddS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
#endif

	TestSZFlags(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]), relativePipeTime);
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

void VUShared::ADDAi(CMipsJitter* codeGen, uint8 dest, uint8 fs)
{
	ADDA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2I),
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

void VUShared::DIV(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf, uint32 relativePipeTime)
{
	size_t destination = g_pipeInfoQ.heldValue;
	QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_DIV, relativePipeTime);

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

void VUShared::FTOI15(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushCstExpand(32768.0f);
	codeGen->MD_MulS();
	codeGen->MD_ToWordTruncate();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::IADD(CMipsJitter* codeGen, uint8 id, uint8 is, uint8 it)
{
	if(id == 0) return;

	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[is]));
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
	codeGen->Add();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[id]));
}

void VUShared::IADDI(CMipsJitter* codeGen, uint8 it, uint8 is, uint8 imm5)
{
	if(it == 0) return;

	PushIntegerRegister(codeGen, is);
	codeGen->PushCst(imm5 | ((imm5 & 0x10) != 0 ? 0xFFFFFFE0 : 0x0));
	codeGen->Add();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
}

void VUShared::IAND(CMipsJitter* codeGen, uint8 id, uint8 is, uint8 it)
{
	if(id == 0) return;

	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[is]));
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
	codeGen->And();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[id]));
}

void VUShared::ILWbase(CMipsJitter* codeGen, uint8 it)
{
	codeGen->LoadFromRef();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
}

void VUShared::ILWR(CMipsJitter* codeGen, uint8 dest, uint8 it, uint8 is, uint32 baseAddress)
{
	//Compute address
	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	ComputeMemAccessAddr(codeGen, is, 0, GetDestOffset(dest));
	codeGen->AddRef();

	ILWbase(codeGen, it);
}

void VUShared::IOR(CMipsJitter* codeGen, uint8 id, uint8 is, uint8 it)
{
	if(id == 0) return;

	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[is]));
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
	codeGen->Or();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[id]));
}

void VUShared::ISUB(CMipsJitter* codeGen, uint8 id, uint8 is, uint8 it)
{
	if(id == 0) return;

	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[is]));
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
	codeGen->Sub();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[id]));
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

void VUShared::ISWbase(CMipsJitter* codeGen, uint8 dest)
{
	for(unsigned int i = 0; i < 4; i++)
	{
		if(VUShared::DestinationHasElement(static_cast<uint8>(dest), i))
		{
			codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
			codeGen->PushIdx(1);		//Push computed address
			codeGen->AddRef();			//Make new ref value
			codeGen->PushIdx(2);		//Push value to store
			codeGen->StoreAtRef();
		}

		if(i != 3)
		{
			codeGen->PushCst(4);
			codeGen->Add();
		}
	}

	codeGen->PullTop();
	codeGen->PullTop();
}

void VUShared::ISWR(CMipsJitter* codeGen, uint8 dest, uint8 it, uint8 is, uint32 baseAddress)
{
	//Compute value to store
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
	codeGen->PushCst(0xFFFF);
	codeGen->And();

	//Compute address
	VUShared::ComputeMemAccessAddr(codeGen, is, 0, 0);

	ISWbase(codeGen, dest);
}

void VUShared::LQbase(CMipsJitter* codeGen, uint8 dest, uint8 it)
{
	if(dest == 0xF)
	{
		codeGen->MD_LoadFromRef();
		codeGen->MD_PullRel(offsetof(CMIPS, m_State.nCOP2[it]));
	}
	else
	{
		for(unsigned int i = 0; i < 4; i++)
		{
			if(VUShared::DestinationHasElement(static_cast<uint8>(dest), i))
			{
				codeGen->PushTop();
				codeGen->LoadFromRef();
				codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[it].nV[i]));
			}

			if(i != 3)
			{
				codeGen->PushCst(4);
				codeGen->AddRef();
			}
		}

		codeGen->PullTop();
	}
}

void VUShared::LQI(CMipsJitter* codeGen, uint8 dest, uint8 it, uint8 is, uint32 baseAddress)
{
	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	VUShared::ComputeMemAccessAddr(codeGen, is, 0, 0);
	codeGen->AddRef();

	VUShared::LQbase(codeGen, dest, it);

	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[is]));
	codeGen->PushCst(1);
	codeGen->Add();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[is]));
}

void VUShared::MADD(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime)
{
	MADD_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fd]),
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft]),
		false, relativePipeTime);
}

void VUShared::MADDbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime)
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
		true, relativePipeTime);
}

void VUShared::MADDi(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime)
{
	MADD_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fd]),
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2I),
		true, relativePipeTime);
}

void VUShared::MADDq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime)
{
	MADD_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fd]),
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2Q),
		true, relativePipeTime);
}

void VUShared::MADDA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint32 relativePipeTime)
{
	MADDA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft]),
		false, relativePipeTime);
}

void VUShared::MADDAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime)
{
	MADDA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
		true, relativePipeTime);
}

void VUShared::MADDAi(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime)
{
	MADDA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2I),
		true, relativePipeTime);
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

void VUShared::MAXi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
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

void VUShared::MSUB(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime)
{
	if(fd == 0)
	{
		//Use the temporary register to store the result
		fd = 32;
	}

	MSUB_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fd]),
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft]),
		false);

	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[fd]), relativePipeTime);
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

void VUShared::MSUBA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint32 relativePipeTime)
{
	MSUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft]),
		false, relativePipeTime);
}

void VUShared::MSUBAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime)
{
	MSUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
		true, relativePipeTime);
}

void VUShared::MSUBAi(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime)
{
	MSUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2I),
		true, relativePipeTime);
}

void VUShared::MFIR(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 is)
{
	for(unsigned int i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(dest, i)) continue;

		PushIntegerRegister(codeGen, is);
		codeGen->SignExt16();
		codeGen->PullRel(VUShared::GetVectorElement(ft, i));
	}
}

void VUShared::MTIR(CMipsJitter* codeGen, uint8 it, uint8 fs, uint8 fsf)
{
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[fs].nV[fsf]));
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
}

void VUShared::MUL(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
	codeGen->MD_MulS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
}

void VUShared::MULbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc, uint32 relativePipeTime)
{
	if(nFd == 0)
	{
		//Use the temporary register to store the result
		nFd = 32;
	}

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[nFt].nV[nBc]));
	codeGen->MD_MulS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));

	TestSZFlags(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]), relativePipeTime);
}

void VUShared::MULi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs)
{
#if 0
	for(unsigned int i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(nDest, i)) continue;

		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2I));
		codeGen->Call(reinterpret_cast<void*>(&FpMulTruncate), 2, true);
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[nFd].nV[i]));
	}
#else
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
	codeGen->MD_MulS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
#endif
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

void VUShared::MULAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]));
	codeGen->MD_MulS();
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A));
	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A), relativePipeTime);
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

void VUShared::OPMSUB(CMipsJitter* codeGen, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime)
{
	//We keep the value in a temp register because it's possible to specify a FD which can be used as FT or FS
	uint8 tempRegIndex = 32;

	//X
	codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPX));
	codeGen->FP_PushSingle(GetVectorElement(fs, VECTOR_COMPY));
	codeGen->FP_PushSingle(GetVectorElement(ft, VECTOR_COMPZ));
	codeGen->FP_Mul();
	codeGen->FP_Sub();
	codeGen->FP_PullSingle(GetVectorElement(tempRegIndex, VECTOR_COMPX));

	//Y
	codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPY));
	codeGen->FP_PushSingle(GetVectorElement(fs, VECTOR_COMPZ));
	codeGen->FP_PushSingle(GetVectorElement(ft, VECTOR_COMPX));
	codeGen->FP_Mul();
	codeGen->FP_Sub();
	codeGen->FP_PullSingle(GetVectorElement(tempRegIndex, VECTOR_COMPY));

	//Z
	codeGen->FP_PushSingle(GetAccumulatorElement(VECTOR_COMPZ));
	codeGen->FP_PushSingle(GetVectorElement(fs, VECTOR_COMPX));
	codeGen->FP_PushSingle(GetVectorElement(ft, VECTOR_COMPY));
	codeGen->FP_Mul();
	codeGen->FP_Sub();
	codeGen->FP_PullSingle(GetVectorElement(tempRegIndex, VECTOR_COMPZ));

	TestSZFlags(codeGen, 0xF, offsetof(CMIPS, m_State.nCOP2[tempRegIndex]), relativePipeTime);

	if(fd != 0)
	{
		codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[tempRegIndex]));
		codeGen->MD_PullRel(offsetof(CMIPS, m_State.nCOP2[fd]));
	}
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

void VUShared::RSQRT(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf, uint32 relativePipeTime)
{
	size_t destination = g_pipeInfoQ.heldValue;
	QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_RSQRT, relativePipeTime);

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

void VUShared::SQbase(CMipsJitter* codeGen, uint8 dest, uint8 is)
{
	if(dest == 0xF)
	{
		codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[is]));
		codeGen->MD_StoreAtRef();
	}
	else
	{
		for(unsigned int i = 0; i < 4; i++)
		{
			if(VUShared::DestinationHasElement(static_cast<uint8>(dest), i))
			{
				codeGen->PushTop();
				codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[is].nV[i]));
				codeGen->StoreAtRef();
			}

			if(i != 3)
			{
				codeGen->PushCst(4);
				codeGen->AddRef();
			}
		}

		codeGen->PullTop();
	}
}

void VUShared::SQD(CMipsJitter* codeGen, uint8 dest, uint8 is, uint8 it, uint32 baseAddress)
{
	//Decrement
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
	codeGen->PushCst(1);
	codeGen->Sub();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[it]));

	//Store
	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	ComputeMemAccessAddr(codeGen, it, 0, 0);
	codeGen->AddRef();

	VUShared::SQbase(codeGen, dest, is);
}

void VUShared::SQI(CMipsJitter* codeGen, uint8 dest, uint8 is, uint8 it, uint32 baseAddress)
{
	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	ComputeMemAccessAddr(codeGen, it, 0, 0);
	codeGen->AddRef();

	VUShared::SQbase(codeGen, dest, is);

	//Increment
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
	codeGen->PushCst(1);
	codeGen->Add();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
}

void VUShared::SQRT(CMipsJitter* codeGen, uint8 nFt, uint8 nFtf, uint32 relativePipeTime)
{
	size_t destination = g_pipeInfoQ.heldValue;
	QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_SQRT, relativePipeTime);

	codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
	codeGen->FP_Sqrt();
	codeGen->FP_PullSingle(destination);
}

void VUShared::SUB(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint32 relativePipeTime)
{
	if(nFd == 0)
	{
		//Use the temporary register to store the result
		nFd = 32;
	}

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFt]));
	codeGen->MD_SubS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));

	TestSZFlags(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]), relativePipeTime);
}

void VUShared::SUBbc(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint8 nFt, uint8 nBc, uint32 relativePipeTime)
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

	TestSZFlags(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]), relativePipeTime);
}

void VUShared::SUBi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint32 relativePipeTime)
{
	if(nFd == 0)
	{
		//Use the temporary register to store the result
		nFd = 32;
	}

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
	codeGen->MD_SubS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));

	TestSZFlags(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]), relativePipeTime);
}

void VUShared::SUBq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2Q));
	codeGen->MD_SubS();
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[fd]));
}

void VUShared::SUBA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft)
{
	SUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft]),
		false);
}

void VUShared::SUBAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc)
{
	SUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
		true);
}

void VUShared::SUBAi(CMipsJitter* codeGen, uint8 dest, uint8 fs)
{
	SUBA_base(codeGen, dest,
		offsetof(CMIPS, m_State.nCOP2[fs]),
		offsetof(CMIPS, m_State.nCOP2I),
		true);
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

void VUShared::CheckPipeline(const PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 relativePipeTime)
{
	codeGen->PushRel(pipeInfo.counter);

	codeGen->PushRel(offsetof(CMIPS, m_State.pipeTime));
	codeGen->PushCst(relativePipeTime);
	codeGen->Add();

	codeGen->BeginIf(Jitter::CONDITION_LE);
	{
		FlushPipeline(pipeInfo, codeGen);
	}
	codeGen->EndIf();
}

void VUShared::QueueInPipeline(const PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 latency, uint32 relativePipeTime)
{
	//Set target
	codeGen->PushRel(offsetof(CMIPS, m_State.pipeTime));
	codeGen->PushCst(relativePipeTime + latency);
	codeGen->Add();
	codeGen->PullRel(pipeInfo.counter);
}

void VUShared::CheckMacFlagPipeline(CMipsJitter* codeGen, uint32 relativePipeTime)
{
	for(unsigned int i = 0; i < MACFLAG_PIPELINE_SLOTS; i++)
	{
		codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeMac.pipeTimes));
		
		//Compute index into array
		codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.index));
		codeGen->PushCst(i);
		codeGen->Add();
		codeGen->PushCst(MACFLAG_PIPELINE_SLOTS - 1);
		codeGen->And();
		
		codeGen->Shl(2);
		codeGen->AddRef();
		codeGen->LoadFromRef();

		codeGen->PushRel(offsetof(CMIPS, m_State.pipeTime));
		codeGen->PushCst(relativePipeTime);
		codeGen->Add();

		codeGen->BeginIf(Jitter::CONDITION_LE);
		{
			codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeMac.values));
		
			//Compute index into array
			codeGen->PushRel(offsetof(CMIPS, m_State.pipeMac.index));
			codeGen->PushCst(i);
			codeGen->Add();
			codeGen->PushCst(MACFLAG_PIPELINE_SLOTS - 1);
			codeGen->And();
		
			codeGen->Shl(2);
			codeGen->AddRef();
			codeGen->LoadFromRef();

			codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2MF));
		}
		codeGen->EndIf();
	}
}
