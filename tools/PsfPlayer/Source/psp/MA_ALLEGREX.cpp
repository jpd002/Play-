#include "MA_ALLEGREX.h"
#include "Jitter.h"
#include "MIPS.h"
#include "offsetof_def.h"

CMA_ALLEGREX::CMA_ALLEGREX()
: CMA_MIPSIV(MIPS_REGSIZE_32)
{
	for(unsigned int i = 0; i < MAX_SPECIAL3_OPS; i++)
	{
		m_pOpSpecial3[i] = std::bind(&CMA_ALLEGREX::Illegal, this);
	}

	for(unsigned int i = 0; i < MAX_BSHFL_OPS; i++)
	{
		m_pOpBshfl[i] = std::bind(&CMA_ALLEGREX::Illegal, this);
	}

	m_pOpGeneral[0x1F] = std::bind(&CMA_ALLEGREX::SPECIAL3, this);

	//Setup SPECIAL
	m_pOpSpecial[0x2C] = std::bind(&CMA_ALLEGREX::MAX, this);
	m_pOpSpecial[0x2D] = std::bind(&CMA_ALLEGREX::MIN, this);

	//Setup SPECIAL3
	m_pOpSpecial3[0x00] = std::bind(&CMA_ALLEGREX::EXT, this);
	m_pOpSpecial3[0x04] = std::bind(&CMA_ALLEGREX::INS, this);
	m_pOpSpecial3[0x20] = std::bind(&CMA_ALLEGREX::BSHFL, this);

	//Setup BSHFL
	m_pOpBshfl[0x10] = std::bind(&CMA_ALLEGREX::SEB, this);
	m_pOpBshfl[0x18] = std::bind(&CMA_ALLEGREX::SEH, this);

	SetupReflectionTables();
}

CMA_ALLEGREX::~CMA_ALLEGREX()
{

}

void CMA_ALLEGREX::SPECIAL3()
{
	m_pOpSpecial3[m_nImmediate & 0x3F]();
}

////////////////////////////////////////////////
// SPECIAL
////////////////////////////////////////////////

//2C
void CMA_ALLEGREX::MAX()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	m_codeGen->BeginIf(Jitter::CONDITION_LE);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	m_codeGen->Else();
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	m_codeGen->EndIf();
}

//2D
void CMA_ALLEGREX::MIN()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	m_codeGen->BeginIf(Jitter::CONDITION_LE);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	m_codeGen->Else();
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	m_codeGen->EndIf();
}

////////////////////////////////////////////////
// SPECIAL3
////////////////////////////////////////////////

//00
void CMA_ALLEGREX::EXT()
{
	uint8 size	= static_cast<uint8>((m_nOpcode >> 11) & 0x001F);
	uint8 pos	= static_cast<uint8>((m_nOpcode >>  6) & 0x001F);

	assert(size != 0);
	size = size + 1;

	uint32 srcMsk = static_cast<uint32>(~0) >> (32 - size);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->Srl(pos);

	m_codeGen->PushCst(srcMsk);
	m_codeGen->And();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//04
void CMA_ALLEGREX::INS()
{
	uint8 size	= static_cast<uint8>((m_nOpcode >> 11) & 0x001F);
	uint8 pos	= static_cast<uint8>((m_nOpcode >>  6) & 0x001F);

	size = size - pos + 1;
	assert(size != 0);

	uint32 srcMsk = static_cast<uint32>(~0) >> (32 - size);
	uint32 dstMsk = static_cast<uint32>(~0) & ~((srcMsk) << pos);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(srcMsk);
	m_codeGen->And();
	m_codeGen->Shl(pos);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushCst(dstMsk);
	m_codeGen->And();

	m_codeGen->Or();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//20
void CMA_ALLEGREX::BSHFL()
{
	m_pOpBshfl[(m_nImmediate >> 6) & 0x1F]();
}

////////////////////////////////////////////////
// BSHFL
////////////////////////////////////////////////

//10
void CMA_ALLEGREX::SEB()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->SignExt8();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//18
void CMA_ALLEGREX::SEH()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->SignExt16();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}
