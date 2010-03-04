#include "MA_ALLEGREX.h"
#include "CodeGen.h"
#include "MIPS.h"

CMA_ALLEGREX::CMA_ALLEGREX()
: CMA_MIPSIV(MIPS_REGSIZE_32)
{
	for(unsigned int i = 0; i < MAX_SPECIAL2_OPS; i++)
	{
		m_pOpSpecial3[i] = std::tr1::bind(&CMA_ALLEGREX::Illegal, this);
	}

	m_pOpGeneral[0x1F] = std::tr1::bind(&CMA_ALLEGREX::SPECIAL3, this);

	//Setup SPECIAL3
	m_pOpSpecial3[0x04] = std::tr1::bind(&CMA_ALLEGREX::INS, this);

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
// SPECIAL3
////////////////////////////////////////////////

//04
void CMA_ALLEGREX::INS()
{
	uint8 size	= static_cast<uint8>((m_nOpcode >> 11) & 0x001F);
	uint8 pos	= static_cast<uint8>((m_nOpcode >>  6) & 0x001F);

	assert(size != 0);
	size = size - pos + 1;

	uint32 srcMsk = static_cast<uint32>(~0) >> (32 - size);
	uint32 dstMsk = static_cast<uint32>(~0) & ((~srcMsk) << pos);

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
