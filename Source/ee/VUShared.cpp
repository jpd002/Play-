#include "VUShared.h"
#include "BitManip.h"
#include "../MIPS.h"
#include "offsetof_def.h"
#include "FpAddTruncate.h"
#include "../FpUtils.h"
#include "Vpu.h"

#define STATUS_Z 0x01
#define STATUS_S 0x02
#define STATUS_I 0x10
#define STATUS_D 0x20
#define STATUS_ZS 0x40
#define STATUS_SS 0x80

// clang-format off
const VUShared::REGISTER_PIPEINFO VUShared::g_pipeInfoQ =
{
	offsetof(CMIPS, m_State.nCOP2Q),
	offsetof(CMIPS, m_State.pipeQ.heldValue),
	offsetof(CMIPS, m_State.pipeQ.counter)
};

const VUShared::REGISTER_PIPEINFO VUShared::g_pipeInfoP =
{
	offsetof(CMIPS, m_State.nCOP2P),
	offsetof(CMIPS, m_State.pipeP.heldValue),
	offsetof(CMIPS, m_State.pipeP.counter)
};

const VUShared::FLAG_PIPEINFO VUShared::g_pipeInfoMac =
{
	offsetof(CMIPS, m_State.nCOP2MF),
	offsetof(CMIPS, m_State.pipeMac.index),
	offsetof(CMIPS, m_State.pipeMac.values),
	offsetof(CMIPS, m_State.pipeMac.pipeTimes)
};

const VUShared::FLAG_PIPEINFO VUShared::g_pipeInfoSticky =
{
	offsetof(CMIPS, m_State.nCOP2SF),
	offsetof(CMIPS, m_State.pipeSticky.index),
	offsetof(CMIPS, m_State.pipeSticky.values),
	offsetof(CMIPS, m_State.pipeSticky.pipeTimes)
};

const VUShared::FLAG_PIPEINFO VUShared::g_pipeInfoClip =
{
	offsetof(CMIPS, m_State.nCOP2CF),
	offsetof(CMIPS, m_State.pipeClip.index),
	offsetof(CMIPS, m_State.pipeClip.values),
	offsetof(CMIPS, m_State.pipeClip.pipeTimes)
};
// clang-format on

using namespace VUShared;

bool VUShared::DestinationHasElement(uint8 nDest, unsigned int nElement)
{
	return (nDest & (1 << (nElement ^ 0x03))) != 0;
}

void VUShared::ComputeMemAccessAddr(CMipsJitter* codeGen, unsigned int baseRegister, uint32 baseOffset, uint32 destOffset, uint32 addressMask)
{
	PushIntegerRegister(codeGen, baseRegister);
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
	codeGen->PushCst(addressMask);
	codeGen->And();
}

uint32 VUShared::GetDestOffset(uint8 dest)
{
	assert(__builtin_popcount(dest) == 1);

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

void TestVectorNaN(CMIPS* context, uint32 dest, uint32 offset)
{
	for(int i = 0; i < 4; i++)
	{
		if(!DestinationHasElement(dest, i)) continue;
		FRAMEWORK_MAYBE_UNUSED uint32 value = reinterpret_cast<uint32*>(context)[(offset / 4) + i];
		assert((value & 0x7F800000) != 0x7F800000);
	}
}

void CheckVectorNaN(CMipsJitter* codeGen, uint8 dest, size_t vector)
{
	codeGen->PushCtx();
	codeGen->PushCst(dest);
	codeGen->PushCst(vector);
	codeGen->Call(reinterpret_cast<void*>(&TestVectorNaN), 3, Jitter::CJitter::RETURN_VALUE_NONE);
}

void VUShared::PullVector(CMipsJitter* codeGen, uint8 dest, size_t vector)
{
	if(dest == 0)
	{
		codeGen->PullTop();
		return;
	}
	assert(vector != offsetof(CMIPS, m_State.nCOP2[0]));
	codeGen->MD_PullRel(vector,
	                    DestinationHasElement(dest, 0),
	                    DestinationHasElement(dest, 1),
	                    DestinationHasElement(dest, 2),
	                    DestinationHasElement(dest, 3));
}

void VUShared::PushIntegerRegister(CMipsJitter* codeGen, unsigned int regIdx)
{
	regIdx &= 0xF;
	if(regIdx == 0)
	{
		codeGen->PushCst(0);
	}
	else
	{
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[regIdx]));
	}
}

void VUShared::PullIntegerRegister(CMipsJitter* codeGen, unsigned int regIdx)
{
	regIdx &= 0xF;
	assert(regIdx != 0);
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[regIdx]));
}

void VUShared::TestSZFlags(CMipsJitter* codeGen, uint8 dest, size_t regOffset, uint32 relativePipeTime, uint32 compileHints)
{
	codeGen->MD_PushRel(regOffset);
	codeGen->MD_MakeSignZero();

	//Clear flags of inactive FMAC units
	if(dest != 0xF)
	{
		codeGen->PushCst((dest << 4) | dest);
		codeGen->And();
	}

	//Update sticky flags
	{
		codeGen->PushTop();

		//Load previous value
		{
			codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeSticky.values));

			codeGen->PushRel(offsetof(CMIPS, m_State.pipeSticky.index));
			codeGen->PushCst(1);
			codeGen->Sub();
			codeGen->PushCst(FLAG_PIPELINE_SLOTS - 1);
			codeGen->And();

			codeGen->Shl(2);
			codeGen->AddRef();
			codeGen->LoadFromRef();
		}

		codeGen->Or();
		QueueInFlagPipeline(g_pipeInfoSticky, codeGen, LATENCY_MAC, relativePipeTime);
	}

	if((compileHints & COMPILEHINT_SKIPFMACUPDATE) == 0)
	{
		QueueInFlagPipeline(g_pipeInfoMac, codeGen, LATENCY_MAC, relativePipeTime);
	}
	else
	{
		codeGen->PullTop();
	}
}

void VUShared::GetStatus(CMipsJitter* codeGen, size_t dstOffset, uint32 relativePipeTime)
{
	//Get STATUS flag using information from other values (MACflags and sticky flags)

	CheckFlagPipeline(g_pipeInfoMac, codeGen, relativePipeTime);
	CheckFlagPipeline(g_pipeInfoSticky, codeGen, relativePipeTime);

	//Reset result
	codeGen->PushCst(0);
	codeGen->PullRel(dstOffset);

	//Check Z flag
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2MF));
	codeGen->PushCst(0x000F);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		codeGen->PushRel(dstOffset);
		codeGen->PushCst(STATUS_Z);
		codeGen->Or();
		codeGen->PullRel(dstOffset);
	}
	codeGen->EndIf();

	//Check S flag
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2MF));
	codeGen->PushCst(0x00F0);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		codeGen->PushRel(dstOffset);
		codeGen->PushCst(STATUS_S);
		codeGen->Or();
		codeGen->PullRel(dstOffset);
	}
	codeGen->EndIf();

	//Check ZS flag
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2SF));
	codeGen->PushCst(0x000F);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		codeGen->PushRel(dstOffset);
		codeGen->PushCst(STATUS_ZS);
		codeGen->Or();
		codeGen->PullRel(dstOffset);
	}
	codeGen->EndIf();

	//Check SS flag
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2SF));
	codeGen->PushCst(0x00F0);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		codeGen->PushRel(dstOffset);
		codeGen->PushCst(STATUS_SS);
		codeGen->Or();
		codeGen->PullRel(dstOffset);
	}
	codeGen->EndIf();

	//Check D flag
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2DF));
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		codeGen->PushRel(dstOffset);
		codeGen->PushCst(STATUS_D);
		codeGen->Or();
		codeGen->PullRel(dstOffset);
	}
	codeGen->EndIf();

	//TODO: Check other flags
}

void VUShared::SetStatus(CMipsJitter* codeGen, size_t srcOffset)
{
	//Only sticky flags can be set

	//Clear sticky flags
	codeGen->PushCst(0);
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2SF));

	codeGen->PushRel(srcOffset);
	codeGen->PushCst(STATUS_ZS);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		codeGen->PushCst(0x000F);
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2SF));
		codeGen->Or();
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2SF));
	}
	codeGen->EndIf();

	codeGen->PushRel(srcOffset);
	codeGen->PushCst(STATUS_SS);
	codeGen->And();
	codeGen->PushCst(0);
	codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		codeGen->PushCst(0x00F0);
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2SF));
		codeGen->Or();
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2SF));
	}
	codeGen->EndIf();

	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2SF));
	ResetFlagPipeline(VUShared::g_pipeInfoSticky, codeGen);
}

void VUShared::ADD_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
{
	codeGen->MD_PushRel(fs);
	codeGen->MD_ClampS();
	if(expand)
	{
		codeGen->MD_PushRelExpand(ft);
	}
	else
	{
		codeGen->MD_PushRel(ft);
	}
	codeGen->MD_AddS();
	PullVector(codeGen, dest, fd);
	TestSZFlags(codeGen, dest, fd, relativePipeTime, compileHints);
}

void VUShared::ADDA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
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
	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A), relativePipeTime, compileHints);
}

void VUShared::MADD_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
	codeGen->MD_PushRel(fs);
	//Clamping is needed by Baldur's Gate Deadly Alliance here because it multiplies junk values (potentially NaN/INF) by 0
	codeGen->MD_ClampS();
	if(expand)
	{
		codeGen->MD_PushRelExpand(ft);
		codeGen->MD_ClampS(); //Fatal Frame 1's door-blocking bug can be fixed by this
	}
	else
	{
		codeGen->MD_PushRel(ft);
	}
	codeGen->MD_MulS();
	codeGen->MD_AddS();
	PullVector(codeGen, dest, fd);
	TestSZFlags(codeGen, dest, fd, relativePipeTime, compileHints);
}

void VUShared::MADDA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
{
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2A));
	codeGen->MD_PushRel(fs);
	//Clamping is needed by Dynasty Warriors 2 here because it multiplies junk values (potentially NaN/INF) by some other value
	codeGen->MD_ClampS();
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
	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A), relativePipeTime, compileHints);
}

void VUShared::SUB_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
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
	PullVector(codeGen, dest, fd);
	TestSZFlags(codeGen, dest, fd, relativePipeTime, compileHints);
}

void VUShared::SUBA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
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
	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A), relativePipeTime, compileHints);
}

void VUShared::MSUB_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
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
	TestSZFlags(codeGen, dest, fd, relativePipeTime, compileHints);
}

void VUShared::MSUBA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
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
	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A), relativePipeTime, compileHints);
}

void VUShared::MUL_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
{
	codeGen->MD_PushRel(fs);
	codeGen->MD_ClampS();
	if(expand)
	{
		codeGen->MD_PushRelExpand(ft);
	}
	else
	{
		codeGen->MD_PushRel(ft);
	}
	codeGen->MD_ClampS();
	codeGen->MD_MulS();
	PullVector(codeGen, dest, fd);
	TestSZFlags(codeGen, dest, fd, relativePipeTime, compileHints);
}

void VUShared::MULA_base(CMipsJitter* codeGen, uint8 dest, size_t fs, size_t ft, bool expand, uint32 relativePipeTime, uint32 compileHints)
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
	codeGen->MD_MulS();
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A));
	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2A), relativePipeTime, compileHints);
}

void VUShared::MINI_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand)
{
	const auto pushFt = [&]() {
		if(expand)
		{
			codeGen->MD_PushRelExpand(ft);
		}
		else
		{
			codeGen->MD_PushRel(ft);
		}
	};

	codeGen->MD_PushRel(fs);
	codeGen->MD_ClampS();
	pushFt();
	codeGen->MD_ClampS();

	codeGen->MD_CmpLtS();
	auto cmp = codeGen->GetTopCursor();

	//Mask FT
	codeGen->PushTop();
	codeGen->MD_Not();
	pushFt();
	codeGen->MD_And();

	//Mask FS
	codeGen->PushCursor(cmp);
	codeGen->MD_PushRel(fs);
	codeGen->MD_And();

	codeGen->MD_Or();
	PullVector(codeGen, dest, fd);

	codeGen->PullTop();
}

void VUShared::MAX_base(CMipsJitter* codeGen, uint8 dest, size_t fd, size_t fs, size_t ft, bool expand)
{
	const auto pushFt = [&]() {
		if(expand)
		{
			codeGen->MD_PushRelExpand(ft);
		}
		else
		{
			codeGen->MD_PushRel(ft);
		}
	};

	codeGen->MD_PushRel(fs);
	codeGen->MD_ClampS();
	pushFt();
	codeGen->MD_ClampS();

	codeGen->MD_CmpGtS();
	auto cmp = codeGen->GetTopCursor();

	//Mask FT
	codeGen->PushTop();
	codeGen->MD_Not();
	pushFt();
	codeGen->MD_And();

	//Mask FS
	codeGen->PushCursor(cmp);
	codeGen->MD_PushRel(fs);
	codeGen->MD_And();

	codeGen->MD_Or();
	PullVector(codeGen, dest, fd);

	codeGen->PullTop();
}

void VUShared::ABS(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	if(nFt == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_AbsS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::ADD(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	ADD_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2[ft]),
	         false, relativePipeTime, compileHints);
}

void VUShared::ADDbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	if((fd != 0) && (ft == 0) && (bc != 3))
	{
		//Using VF0 (other than W component) as broadcast value, which basically means add with 0.
		//Do a simple move to the destination register to avoid clamping which can alter the other addend.
		//Piposaru 2001 uses this to move some integer values (0xFFFF8000) between VF regs.
		codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
		PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[fd]));
		TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[fd]), relativePipeTime, compileHints);
	}
	else
	{
		ADD_base(codeGen, dest,
		         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
		         offsetof(CMIPS, m_State.nCOP2[fs]),
		         offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
		         true, relativePipeTime, compileHints);
	}
}

void VUShared::ADDi(CMipsJitter* codeGen, uint8 nDest, uint8 nFd, uint8 nFs, uint32 relativePipeTime, uint32 compileHints)
{
	if(nFd == 0)
	{
		//Use the temporary register to store the result
		nFd = 32;
	}

	//Use FpAddTruncate on all platforms, except JavaScript
	//On JavaScript, using it doesn't seem to help Tri-Ace games
	//there's probably some other rounding issue on that platform
#if !defined(__EMSCRIPTEN__)
	for(unsigned int i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(nDest, i)) continue;

		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[nFs].nV[i]));
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2I));
		codeGen->Call(reinterpret_cast<void*>(&FpAddTruncate), 2, Jitter::CJitter::RETURN_VALUE_32);
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[nFd].nV[i]));
	}
#else
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2I));
	codeGen->MD_AddS();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]));
#endif

	TestSZFlags(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFd]), relativePipeTime, compileHints);
}

void VUShared::ADDq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	ADD_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2Q),
	         true, relativePipeTime, compileHints);
}

void VUShared::ADDA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	ADDA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft]),
	          false, relativePipeTime, compileHints);
}

void VUShared::ADDAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	ADDA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	          true, relativePipeTime, compileHints);
}

void VUShared::ADDAi(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	ADDA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2I),
	          true, relativePipeTime, compileHints);
}

void VUShared::ADDAq(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	ADDA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2Q),
	          true, relativePipeTime, compileHints);
}

void VUShared::CLIP(CMipsJitter* codeGen, uint8 nFs, uint8 nFt, uint32 relativePipeTime)
{
	size_t tempOffset = offsetof(CMIPS, m_State.nCOP2T);

	//Load previous value
	{
		codeGen->PushRelAddrRef(offsetof(CMIPS, m_State.pipeClip.values));

		codeGen->PushRel(offsetof(CMIPS, m_State.pipeClip.index));
		codeGen->PushCst(1);
		codeGen->Sub();
		codeGen->PushCst(FLAG_PIPELINE_SLOTS - 1);
		codeGen->And();

		codeGen->Shl(2);
		codeGen->AddRef();
		codeGen->LoadFromRef();
		codeGen->PullRel(tempOffset);
	}

	//Create some space for the new test results
	codeGen->PushRel(tempOffset);
	codeGen->Shl(6);
	codeGen->PullRel(tempOffset);

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
			codeGen->PushRel(tempOffset);
			codeGen->PushCst(1 << ((i * 2) + 0));
			codeGen->Or();
			codeGen->PullRel(tempOffset);
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
			codeGen->PushRel(tempOffset);
			codeGen->PushCst(1 << ((i * 2) + 1));
			codeGen->Or();
			codeGen->PullRel(tempOffset);
		}
		codeGen->EndIf();
	}

	codeGen->PushRel(tempOffset);
	QueueInFlagPipeline(g_pipeInfoClip, codeGen, LATENCY_MAC, relativePipeTime);
}

void VUShared::DIV(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf, uint32 relativePipeTime)
{
	size_t destination = g_pipeInfoQ.heldValue;
	QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_DIV, relativePipeTime);

	//Check for zero
	FpUtils::IsZero(codeGen, GetVectorElement(nFt, nFtf));
	codeGen->BeginIf(Jitter::CONDITION_EQ);
	{
		FpUtils::ComputeDivisionByZero(codeGen,
		                               GetVectorElement(nFs, nFsf),
		                               GetVectorElement(nFt, nFtf));
		codeGen->PullRel(destination);

		codeGen->PushCst(1);
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2DF));
	}
	codeGen->Else();
	{
		codeGen->FP_PushSingle(GetVectorElement(nFs, nFsf));
		codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
		codeGen->FP_Div();
		codeGen->FP_PullSingle(destination);

		codeGen->PushCst(0);
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2DF));
	}
	codeGen->EndIf();
}

void VUShared::FTOI0(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	if(nFt == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_ToWordTruncate();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::FTOI4(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	if(nFt == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushCstExpand(16.0f);
	codeGen->MD_MulS();
	codeGen->MD_ToWordTruncate();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::FTOI12(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	if(nFt == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushCstExpand(4096.0f);
	codeGen->MD_MulS();
	codeGen->MD_ToWordTruncate();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::FTOI15(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	if(nFt == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	codeGen->MD_PushCstExpand(32768.0f);
	codeGen->MD_MulS();
	codeGen->MD_ToWordTruncate();
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::IADD(CMipsJitter* codeGen, uint8 id, uint8 is, uint8 it)
{
	if((id & 0xF) == 0) return;

	PushIntegerRegister(codeGen, is);
	PushIntegerRegister(codeGen, it);
	codeGen->Add();
	PullIntegerRegister(codeGen, id);
}

void VUShared::IADDI(CMipsJitter* codeGen, uint8 it, uint8 is, uint8 imm5)
{
	if((it & 0xF) == 0) return;

	PushIntegerRegister(codeGen, is);
	codeGen->PushCst(imm5 | ((imm5 & 0x10) != 0 ? 0xFFFFFFE0 : 0x0));
	codeGen->Add();
	PullIntegerRegister(codeGen, it);
}

void VUShared::IAND(CMipsJitter* codeGen, uint8 id, uint8 is, uint8 it)
{
	if((id & 0xF) == 0) return;

	PushIntegerRegister(codeGen, is);
	PushIntegerRegister(codeGen, it);
	codeGen->And();
	PullIntegerRegister(codeGen, id);
}

void VUShared::ILWbase(CMipsJitter* codeGen, uint8 it)
{
	codeGen->LoadFromRefIdx(1);
	PullIntegerRegister(codeGen, it);
}

void VUShared::ILWR(CMipsJitter* codeGen, uint8 dest, uint8 it, uint8 is, uint32 addressMask)
{
	if((it & 0xF) == 0) return;

	//Compute address
	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));

	VUShared::ComputeMemAccessAddr(codeGen, is, 0, GetDestOffset(dest), addressMask);

	ILWbase(codeGen, it);
}

void VUShared::IOR(CMipsJitter* codeGen, uint8 id, uint8 is, uint8 it)
{
	if((id & 0xF) == 0) return;

	PushIntegerRegister(codeGen, is);
	PushIntegerRegister(codeGen, it);
	codeGen->Or();
	PullIntegerRegister(codeGen, id);
}

void VUShared::ISUB(CMipsJitter* codeGen, uint8 id, uint8 is, uint8 it)
{
	if((id & 0xF) == 0) return;

	PushIntegerRegister(codeGen, is);
	PushIntegerRegister(codeGen, it);
	codeGen->Sub();
	PullIntegerRegister(codeGen, id);
}

void VUShared::ITOF0(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 fs)
{
	if(ft == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
	codeGen->MD_ToSingle();
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[ft]));
}

void VUShared::ITOF4(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 fs)
{
	if(ft == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
	codeGen->MD_ToSingle();
	codeGen->MD_PushCstExpand(16.0f);
	codeGen->MD_DivS();
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[ft]));
}

void VUShared::ITOF12(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 fs)
{
	if(ft == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
	codeGen->MD_ToSingle();
	codeGen->MD_PushCstExpand(4096.0f);
	codeGen->MD_DivS();
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[ft]));
}

void VUShared::ITOF15(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 fs)
{
	if(ft == 0) return;

	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[fs]));
	codeGen->MD_ToSingle();
	codeGen->MD_PushCstExpand(32768.0f);
	codeGen->MD_DivS();
	PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[ft]));
}

void VUShared::ISWbase(CMipsJitter* codeGen, uint8 dest, uint8 it)
{
	codeGen->AddRef();

	//Compute value to store
	PushIntegerRegister(codeGen, it);
	codeGen->PushCst(0xFFFF);
	codeGen->And();

	for(unsigned int i = 0; i < 4; i++)
	{
		if(VUShared::DestinationHasElement(static_cast<uint8>(dest), i))
		{
			codeGen->PushIdx(1);     //Push dest ref
			codeGen->PushCst(i * 4); //Push index
			codeGen->PushIdx(2);     //Push value to store
			codeGen->StoreAtRefIdx(1);
		}
	}

	codeGen->PullTop();
	codeGen->PullTop();
}

void VUShared::ISWR(CMipsJitter* codeGen, uint8 dest, uint8 it, uint8 is, uint32 addressMask)
{
	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));

	//Compute address
	VUShared::ComputeMemAccessAddr(codeGen, is, 0, 0, addressMask);

	ISWbase(codeGen, dest, it);
}

void VUShared::LQbase(CMipsJitter* codeGen, uint8 dest, uint8 it)
{
	if(it == 0)
	{
		codeGen->PullTop();
		codeGen->PullTop();
		return;
	}

	if(dest == 0xF)
	{
		codeGen->MD_LoadFromRefIdx(1);
		codeGen->MD_PullRel(offsetof(CMIPS, m_State.nCOP2[it]));
	}
	else
	{
		codeGen->AddRef();

		//Try some aligned 64-bit reads first
		for(unsigned int i = 0; i < 3; i += 2)
		{
			uint8 mask = (0x03 << i);
			if((dest & mask) == mask)
			{
				unsigned int element = (i ^ 0x03) - 1;
				codeGen->PushTop();
				codeGen->PushCst(element * 4);
				codeGen->Load64FromRefIdx(1);
				codeGen->PullRel64(offsetof(CMIPS, m_State.nCOP2[it].nV[element]));
				dest &= ~mask;
			}
		}

		//Read any remaining data
		for(unsigned int i = 0; i < 4; i++)
		{
			if(VUShared::DestinationHasElement(static_cast<uint8>(dest), i))
			{
				codeGen->PushTop();
				codeGen->PushCst(i * 4);
				codeGen->LoadFromRefIdx(1);
				codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[it].nV[i]));
			}
		}

		codeGen->PullTop();
	}
}

void VUShared::LQD(CMipsJitter* codeGen, uint8 dest, uint8 it, uint8 is, uint32 addressMask)
{
	if((is & 0xF) != 0)
	{
		PushIntegerRegister(codeGen, is);
		codeGen->PushCst(1);
		codeGen->Sub();
		PullIntegerRegister(codeGen, is);
	}

	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	VUShared::ComputeMemAccessAddr(codeGen, is, 0, 0, addressMask);

	VUShared::LQbase(codeGen, dest, it);
}

void VUShared::LQI(CMipsJitter* codeGen, uint8 dest, uint8 it, uint8 is, uint32 addressMask)
{
	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	VUShared::ComputeMemAccessAddr(codeGen, is, 0, 0, addressMask);

	VUShared::LQbase(codeGen, dest, it);

	if((is & 0xF) != 0)
	{
		PushIntegerRegister(codeGen, is);
		codeGen->PushCst(1);
		codeGen->Add();
		PullIntegerRegister(codeGen, is);
	}
}

void VUShared::MADD(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	MADD_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft]),
	          false, relativePipeTime, compileHints);
}

void VUShared::MADDbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	MADD_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	          true, relativePipeTime, compileHints);
}

void VUShared::MADDi(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MADD_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2I),
	          true, relativePipeTime, compileHints);
}

void VUShared::MADDq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MADD_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2Q),
	          true, relativePipeTime, compileHints);
}

void VUShared::MADDA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	MADDA_base(codeGen, dest,
	           offsetof(CMIPS, m_State.nCOP2[fs]),
	           offsetof(CMIPS, m_State.nCOP2[ft]),
	           false, relativePipeTime, compileHints);
}

void VUShared::MADDAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	MADDA_base(codeGen, dest,
	           offsetof(CMIPS, m_State.nCOP2[fs]),
	           offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	           true, relativePipeTime, compileHints);
}

void VUShared::MADDAi(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MADDA_base(codeGen, dest,
	           offsetof(CMIPS, m_State.nCOP2[fs]),
	           offsetof(CMIPS, m_State.nCOP2I),
	           true, relativePipeTime, compileHints);
}

void VUShared::MADDAq(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MADDA_base(codeGen, dest,
	           offsetof(CMIPS, m_State.nCOP2[fs]),
	           offsetof(CMIPS, m_State.nCOP2Q),
	           true, relativePipeTime, compileHints);
}

void VUShared::MAX(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft)
{
	if(fd == 0) return;
	MAX_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[fd]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2[ft]),
	         false);
}

void VUShared::MAXbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc)
{
	if(fd == 0) return;
	MAX_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[fd]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	         true);
}

void VUShared::MAXi(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs)
{
	if(fd == 0) return;
	MAX_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[fd]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2I),
	         true);
}

void VUShared::MINI(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft)
{
	if(fd == 0) return;
	MINI_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fd]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft]),
	          false);
}

void VUShared::MINIbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc)
{
	if(fd == 0) return;
	MINI_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fd]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	          true);
}

void VUShared::MINIi(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs)
{
	if(fd == 0) return;
	MINI_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fd]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2I),
	          true);
}

void VUShared::MOVE(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	if(nFt == 0) return;
	codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[nFs]));
	PullVector(codeGen, nDest, offsetof(CMIPS, m_State.nCOP2[nFt]));
}

void VUShared::MR32(CMipsJitter* codeGen, uint8 nDest, uint8 nFt, uint8 nFs)
{
	if(nFt == 0) return;

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

void VUShared::MSUB(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	MSUB_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft]),
	          false, relativePipeTime, compileHints);
}

void VUShared::MSUBbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	MSUB_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	          true, relativePipeTime, compileHints);
}

void VUShared::MSUBi(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MSUB_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2I),
	          true, relativePipeTime, compileHints);
}

void VUShared::MSUBq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MSUB_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2Q),
	          true, relativePipeTime, compileHints);
}

void VUShared::MSUBA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	MSUBA_base(codeGen, dest,
	           offsetof(CMIPS, m_State.nCOP2[fs]),
	           offsetof(CMIPS, m_State.nCOP2[ft]),
	           false, relativePipeTime, compileHints);
}

void VUShared::MSUBAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	MSUBA_base(codeGen, dest,
	           offsetof(CMIPS, m_State.nCOP2[fs]),
	           offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	           true, relativePipeTime, compileHints);
}

void VUShared::MSUBAi(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MSUBA_base(codeGen, dest,
	           offsetof(CMIPS, m_State.nCOP2[fs]),
	           offsetof(CMIPS, m_State.nCOP2I),
	           true, relativePipeTime, compileHints);
}

void VUShared::MSUBAq(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MSUBA_base(codeGen, dest,
	           offsetof(CMIPS, m_State.nCOP2[fs]),
	           offsetof(CMIPS, m_State.nCOP2Q),
	           true, relativePipeTime, compileHints);
}

void VUShared::MFIR(CMipsJitter* codeGen, uint8 dest, uint8 ft, uint8 is)
{
	if(ft == 0) return;
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
	if((it & 0xF) == 0) return;
	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[fs].nV[fsf]));
	PullIntegerRegister(codeGen, it);
}

void VUShared::MUL(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	MUL_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2[ft]),
	         false, relativePipeTime, compileHints);
}

void VUShared::MULbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	MUL_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	         true, relativePipeTime, compileHints);
}

void VUShared::MULi(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MUL_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2I),
	         true, relativePipeTime, compileHints);
}

void VUShared::MULq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MUL_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2Q),
	         true, relativePipeTime, compileHints);
}

void VUShared::MULA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	MULA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft]),
	          false, relativePipeTime, compileHints);
}

void VUShared::MULAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	MULA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	          true, relativePipeTime, compileHints);
}

void VUShared::MULAi(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MULA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2I),
	          true, relativePipeTime, compileHints);
}

void VUShared::MULAq(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	MULA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2Q),
	          true, relativePipeTime, compileHints);
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

void VUShared::OPMSUB(CMipsJitter* codeGen, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	//We keep the value in a temp register because it's possible to specify a FD which can be used as FT or FS
	uint8 tempRegIndex = 32;
	uint32 dest = 0x0E;

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

	TestSZFlags(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[tempRegIndex]), relativePipeTime, compileHints);

	if(fd != 0)
	{
		codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[tempRegIndex]));
		PullVector(codeGen, dest, offsetof(CMIPS, m_State.nCOP2[fd]));
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
	if(ft == 0) return;

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
	codeGen->Srl(4);
	codeGen->PushCst(1);
	codeGen->And();

	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2R));
	codeGen->Srl(22);
	codeGen->PushCst(1);
	codeGen->And();

	codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2R));
	codeGen->Shl(1);

	codeGen->Xor();
	codeGen->Xor();

	codeGen->PushCst(0x007FFFFF);
	codeGen->And();
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2R));

	RGET(codeGen, dest, ft);
}

void VUShared::RSQRT(CMipsJitter* codeGen, uint8 nFs, uint8 nFsf, uint8 nFt, uint8 nFtf, uint32 relativePipeTime)
{
	size_t destination = g_pipeInfoQ.heldValue;
	QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_RSQRT, relativePipeTime);

	//Check for zero
	FpUtils::IsZero(codeGen, GetVectorElement(nFt, nFtf));
	codeGen->BeginIf(Jitter::CONDITION_EQ);
	{
		FpUtils::ComputeDivisionByZero(codeGen,
		                               GetVectorElement(nFs, nFsf),
		                               GetVectorElement(nFt, nFtf));
		codeGen->PullRel(destination);

		codeGen->PushCst(1);
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2DF));
	}
	codeGen->Else();
	{
		codeGen->FP_PushSingle(GetVectorElement(nFs, nFsf));
		codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
		codeGen->FP_Rsqrt();
		codeGen->FP_Mul();
		codeGen->FP_PullSingle(destination);

		codeGen->PushCst(0);
		codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2DF));
	}
	codeGen->EndIf();
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
		codeGen->MD_StoreAtRefIdx(1);
	}
	else
	{
		codeGen->AddRef();

		//Try some aligned 64-bit writes first
		for(unsigned int i = 0; i < 3; i += 2)
		{
			uint8 mask = (0x03 << i);
			if((dest & mask) == mask)
			{
				unsigned int element = (i ^ 0x03) - 1;
				codeGen->PushTop();
				codeGen->PushCst(element * 4);
				codeGen->PushRel64(offsetof(CMIPS, m_State.nCOP2[is].nV[element]));
				codeGen->Store64AtRefIdx(1);
				dest &= ~mask;
			}
		}

		//Write any remaining data
		for(unsigned int i = 0; i < 4; i++)
		{
			if(VUShared::DestinationHasElement(static_cast<uint8>(dest), i))
			{
				codeGen->PushTop();      //Push dest memory ref
				codeGen->PushCst(i * 4); //Push index
				codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[is].nV[i]));
				codeGen->StoreAtRefIdx(1);
			}
		}

		codeGen->PullTop();
	}
}

void VUShared::SQD(CMipsJitter* codeGen, uint8 dest, uint8 is, uint8 it, uint32 addressMask)
{
	//Decrement
	if((it & 0xF) != 0)
	{
		PushIntegerRegister(codeGen, it);
		codeGen->PushCst(1);
		codeGen->Sub();
		PullIntegerRegister(codeGen, it);
	}

	//Store
	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	ComputeMemAccessAddr(codeGen, it, 0, 0, addressMask);

	VUShared::SQbase(codeGen, dest, is);
}

void VUShared::SQI(CMipsJitter* codeGen, uint8 dest, uint8 is, uint8 it, uint32 addressMask, const Vu1AreaAccessEmitter& vu1AreaAccessEmitter)
{
	if(vu1AreaAccessEmitter)
	{
		codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[it]));
		codeGen->PushCst(CVpu::VU_ADDR_VU1AREA_START >> 4);
		codeGen->BeginIf(Jitter::CONDITION_AE);
		{
			vu1AreaAccessEmitter(codeGen, is, it);
		}
		codeGen->Else();
	}

	codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	ComputeMemAccessAddr(codeGen, it, 0, 0, addressMask);

	VUShared::SQbase(codeGen, dest, is);

	if(vu1AreaAccessEmitter)
	{
		codeGen->EndIf();
	}

	//Increment
	if((it & 0xF) != 0)
	{
		PushIntegerRegister(codeGen, it);
		codeGen->PushCst(1);
		codeGen->Add();
		PullIntegerRegister(codeGen, it);
	}
}

void VUShared::SQRT(CMipsJitter* codeGen, uint8 nFt, uint8 nFtf, uint32 relativePipeTime)
{
	size_t destination = g_pipeInfoQ.heldValue;
	QueueInPipeline(g_pipeInfoQ, codeGen, LATENCY_SQRT, relativePipeTime);

	codeGen->FP_PushSingle(GetVectorElement(nFt, nFtf));
	codeGen->FP_Abs();
	codeGen->FP_Sqrt();
	codeGen->FP_PullSingle(destination);

	codeGen->PushCst(0);
	codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2DF));
}

void VUShared::SUB(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	auto fdOffset = offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]);
	if(fs == ft)
	{
		//Source and target registers are the same, clear the vector instead of going through a SUB instruction
		//SUB might generate NaNs instead of clearing the values like the game intended (ex.: Homura with 0xFFFF8000)
		codeGen->MD_PushRelExpand(offsetof(CMIPS, m_State.nCOP2[0].nV0));
		PullVector(codeGen, dest, fdOffset);
		TestSZFlags(codeGen, dest, fdOffset, relativePipeTime, compileHints);
	}
	else
	{
		SUB_base(codeGen, dest,
		         fdOffset,
		         offsetof(CMIPS, m_State.nCOP2[fs]),
		         offsetof(CMIPS, m_State.nCOP2[ft]),
		         false, relativePipeTime, compileHints);
	}
}

void VUShared::SUBbc(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	SUB_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	         true, relativePipeTime, compileHints);
}

void VUShared::SUBi(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	SUB_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2I),
	         true, relativePipeTime, compileHints);
}

void VUShared::SUBq(CMipsJitter* codeGen, uint8 dest, uint8 fd, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	SUB_base(codeGen, dest,
	         offsetof(CMIPS, m_State.nCOP2[(fd != 0) ? fd : 32]),
	         offsetof(CMIPS, m_State.nCOP2[fs]),
	         offsetof(CMIPS, m_State.nCOP2Q),
	         true, relativePipeTime, compileHints);
}

void VUShared::SUBA(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint32 relativePipeTime, uint32 compileHints)
{
	SUBA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft]),
	          false, relativePipeTime, compileHints);
}

void VUShared::SUBAbc(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint8 ft, uint8 bc, uint32 relativePipeTime, uint32 compileHints)
{
	SUBA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2[ft].nV[bc]),
	          true, relativePipeTime, compileHints);
}

void VUShared::SUBAi(CMipsJitter* codeGen, uint8 dest, uint8 fs, uint32 relativePipeTime, uint32 compileHints)
{
	SUBA_base(codeGen, dest,
	          offsetof(CMIPS, m_State.nCOP2[fs]),
	          offsetof(CMIPS, m_State.nCOP2I),
	          true, relativePipeTime, compileHints);
}

void VUShared::WAITP(CMipsJitter* codeGen)
{
	FlushPipeline(g_pipeInfoP, codeGen);
}

void VUShared::WAITQ(CMipsJitter* codeGen)
{
	FlushPipeline(g_pipeInfoQ, codeGen);
}

void VUShared::FlushPipeline(const REGISTER_PIPEINFO& pipeInfo, CMipsJitter* codeGen)
{
	codeGen->PushCst(0);
	codeGen->PullRel(pipeInfo.counter);

	codeGen->PushRel(pipeInfo.heldValue);
	codeGen->PullRel(pipeInfo.value);
}

void VUShared::SyncPipeline(const REGISTER_PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 relativePipeTime)
{
	codeGen->PushRel(pipeInfo.counter);

	codeGen->PushRel(offsetof(CMIPS, m_State.pipeTime));
	codeGen->PushCst(relativePipeTime);
	codeGen->Add();

	//If time for value to arrive (pipeInfo.counter) is greater than our current pipeTime,
	//increase pipeTime so that it matches.
	codeGen->BeginIf(Jitter::CONDITION_GE);
	{
		codeGen->PushRel(pipeInfo.counter);
		codeGen->PushCst(relativePipeTime);
		codeGen->Sub();
		codeGen->PullRel(offsetof(CMIPS, m_State.pipeTime));
	}
	codeGen->EndIf();

	FlushPipeline(pipeInfo, codeGen);
}

void VUShared::CheckPipeline(const REGISTER_PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 relativePipeTime)
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

void VUShared::QueueInPipeline(const REGISTER_PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 latency, uint32 relativePipeTime)
{
	//Set target
	codeGen->PushRel(offsetof(CMIPS, m_State.pipeTime));
	codeGen->PushCst(relativePipeTime + latency);
	codeGen->Add();
	codeGen->PullRel(pipeInfo.counter);
}

void VUShared::CheckFlagPipeline(const FLAG_PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 relativePipeTime)
{
	//This will check every slot in the pipeline and update
	//the flag register every time (pipeTimes[i] <= (pipeTime + relativePipeTime))
	for(unsigned int i = 0; i < FLAG_PIPELINE_SLOTS; i++)
	{
		codeGen->PushRelAddrRef(pipeInfo.timeArray);

		//Compute index into array
		codeGen->PushRel(pipeInfo.index);
		codeGen->PushCst(i);
		codeGen->Add();
		codeGen->PushCst(FLAG_PIPELINE_SLOTS - 1);
		codeGen->And();

		codeGen->Shl(2);
		codeGen->AddRef();
		codeGen->LoadFromRef();

		codeGen->PushRel(offsetof(CMIPS, m_State.pipeTime));
		codeGen->PushCst(relativePipeTime);
		codeGen->Add();

		codeGen->BeginIf(Jitter::CONDITION_LE);
		{
			codeGen->PushRelAddrRef(pipeInfo.valueArray);

			//Compute index into array
			codeGen->PushRel(pipeInfo.index);
			codeGen->PushCst(i);
			codeGen->Add();
			codeGen->PushCst(FLAG_PIPELINE_SLOTS - 1);
			codeGen->And();

			codeGen->Shl(2);
			codeGen->AddRef();
			codeGen->LoadFromRef();

			codeGen->PullRel(pipeInfo.value);
		}
		codeGen->EndIf();
	}
}

void VUShared::QueueInFlagPipeline(const FLAG_PIPEINFO& pipeInfo, CMipsJitter* codeGen, uint32 latency, uint32 relativePipeTime)
{
	uint32 valueCursor = codeGen->GetTopCursor();

	//Get offset
	codeGen->PushRel(pipeInfo.index);
	uint32 offsetCursor = codeGen->GetTopCursor();

	//Write time
	{
		//Generate time address
		codeGen->PushRelAddrRef(pipeInfo.timeArray);
		codeGen->PushCursor(offsetCursor);

		//Generate time
		codeGen->PushRel(offsetof(CMIPS, m_State.pipeTime));
		codeGen->PushCst(relativePipeTime + latency);
		codeGen->Add();

		//--- Store time
		codeGen->StoreAtRefIdx();
	}

	//Write value
	{
		//Generate value address
		codeGen->PushRelAddrRef(pipeInfo.valueArray);
		codeGen->PushCursor(offsetCursor);

		//--- Store value
		codeGen->PushCursor(valueCursor);
		codeGen->StoreAtRefIdx();
	}

	assert(codeGen->GetTopCursor() == offsetCursor);
	codeGen->PullTop();
	assert(codeGen->GetTopCursor() == valueCursor);
	codeGen->PullTop();

	//Increment counter
	codeGen->PushRel(pipeInfo.index);
	codeGen->PushCst(1);
	codeGen->Add();
	codeGen->PushCst(FLAG_PIPELINE_SLOTS - 1);
	codeGen->And();
	codeGen->PullRel(pipeInfo.index);
}

void VUShared::ResetFlagPipeline(const FLAG_PIPEINFO& pipeInfo, CMipsJitter* codeGen)
{
	uint32 valueCursor = codeGen->GetTopCursor();

	for(uint32 i = 0; i < FLAG_PIPELINE_SLOTS; i++)
	{
		codeGen->PushRelAddrRef(pipeInfo.timeArray);
		codeGen->PushCst(i);
		codeGen->PushCst(0);
		codeGen->StoreAtRefIdx();

		codeGen->PushRelAddrRef(pipeInfo.valueArray);
		codeGen->PushCst(i);
		codeGen->PushCursor(valueCursor);
		codeGen->StoreAtRefIdx();
	}

	assert(codeGen->GetTopCursor() == valueCursor);
	codeGen->PullTop();
}

void VUShared::CheckFlagPipelineImmediate(const FLAG_PIPEINFO& pipeInfo, CMIPS* context, uint32 relativePipeTime)
{
	auto rawState = reinterpret_cast<uint8*>(&context->m_State);
	auto value = reinterpret_cast<uint32*>(rawState + pipeInfo.value);
	auto pipeIndex = *reinterpret_cast<uint32*>(rawState + pipeInfo.index);
	auto valueArray = reinterpret_cast<uint32*>(rawState + pipeInfo.valueArray);
	auto timeArray = reinterpret_cast<uint32*>(rawState + pipeInfo.timeArray);

	for(unsigned int i = 0; i < FLAG_PIPELINE_SLOTS; i++)
	{
		unsigned int index = (i + pipeIndex) & (FLAG_PIPELINE_SLOTS - 1);
		if(timeArray[index] <= (context->m_State.pipeTime + relativePipeTime))
		{
			*value = valueArray[index];
		}
	}
}

void VUShared::ResetFlagPipelineImmediate(const FLAG_PIPEINFO& pipeInfo, CMIPS* context, uint32 value)
{
	auto rawState = reinterpret_cast<uint8*>(&context->m_State);
	auto valueArray = reinterpret_cast<uint32*>(rawState + pipeInfo.valueArray);
	auto timeArray = reinterpret_cast<uint32*>(rawState + pipeInfo.timeArray);

	for(unsigned int i = 0; i < FLAG_PIPELINE_SLOTS; i++)
	{
		timeArray[i] = 0;
		valueArray[i] = value;
	}
}
