#include "CodeGen_VUF128.h"
#include "PtrMacro.h"
#ifdef AMD64
#include "amd64/CPUID.h"
#endif

using namespace CodeGen;

CCacheBlock*				CVUF128::m_pBlock = NULL;
CVUF128::CFactory*			CVUF128::m_pFactory = CreateFactory();
uint8						CVUF128::m_nRegister = 0;

CVUF128::CFactory* CVUF128::CreateFactory()
{
	uint32 nFeatures;

#ifdef AMD64

	//Quite useless, since most AMD64 CPUs would have SSE2 anyways...
	nFeatures = _cpuid_GetCpuFeatures();

#else

	__asm
	{
		mov eax, 0x00000001;
		cpuid;
		mov dword ptr[nFeatures], edx
	}

#endif

	//Check SSE2
	if(nFeatures & 0x04000000)
	{
		return new CSSE2Factory();
	}

	//Check SSE
	if(nFeatures & 0x02000000)
	{
		return new CSSEFactory();
	}

	return NULL;
}

void CVUF128::Begin(CCacheBlock* pBlock)
{
	m_pBlock = pBlock;
	m_pFactory->Begin();
}

void CVUF128::End()
{
	m_pFactory->End(m_pBlock);
}

void CVUF128::Push(uint128* pVector)
{
	m_pFactory->Push(m_pBlock, pVector);
}

void CVUF128::Push(uint32* pValue)
{
	m_pFactory->Push(m_pBlock, pValue);
}

void CVUF128::PushImm(float nValue)
{
	m_pFactory->PushImm(m_pBlock, nValue);
}

void CVUF128::PushTop()
{
	m_pFactory->PushTop(m_pBlock);
}

void CVUF128::Pull(uint32* pElement0, uint32* pElement1, uint32* pElement2, uint32* pElement3)
{
	m_pFactory->Pull(m_pBlock, pElement0, pElement1, pElement2, pElement3);
}

void CVUF128::Add()
{
	m_pFactory->Add(m_pBlock);
}

void CVUF128::Max()
{
	m_pFactory->Max(m_pBlock);
}

void CVUF128::Min()
{
	m_pFactory->Min(m_pBlock);
}

void CVUF128::Mul()
{
	m_pFactory->Mul(m_pBlock);
}

void CVUF128::Sub()
{
	m_pFactory->Sub(m_pBlock);
}

void CVUF128::Truncate()
{
	m_pFactory->Truncate(m_pBlock);
}

void CVUF128::ToWordTruncate()
{
	m_pFactory->ToWordTruncate(m_pBlock);
}

void CVUF128::IsNegative()
{
	m_pFactory->IsNegative(m_pBlock);
}

void CVUF128::IsZero()
{
	m_pFactory->IsZero(m_pBlock);
}

void CVUF128::ToSingle()
{
	m_pFactory->ToSingle(m_pBlock);
}

/////////////////////////////////////////////
//Factory Implementation
/////////////////////////////////////////////

CVUF128::CFactory::~CFactory()
{

}

/////////////////////////////////////////////
//SSE Factory Implementation
/////////////////////////////////////////////

CVUF128::CSSEFactory::CSSEFactory()
{
	m_nRegister = 0;
}

CVUF128::CSSEFactory::~CSSEFactory()
{

}

void CVUF128::CSSEFactory::Begin()
{
	m_nRegister = 0;
	m_nStackAlloc = 0;
	m_nNeedsEMMS = false;
}

void CVUF128::CSSEFactory::End(CCacheBlock* pBlock)
{
	if(m_nStackAlloc != 0)
	{
		//add esp, m_nStackAlloc
		pBlock->StreamWrite(3, 0x83, 0xC4, m_nStackAlloc);
	}

	if(m_nNeedsEMMS)
	{
		//emms
		pBlock->StreamWrite(2, 0x0F, 0x77);
	}
}

void CVUF128::CSSEFactory::Push(CCacheBlock* pBlock, uint128* pVector)
{
	//movaps xmm?, pVector
	pBlock->StreamWrite(3, 0x0F, 0x28, 0x00 | (m_nRegister << 3) | 0x05);
	pBlock->StreamWriteWord((uint32)((uint8*)(pVector) - (uint8*)0));

	m_nRegister++;
}

void CVUF128::CSSEFactory::Push(CCacheBlock* pBlock, uint32* pValue)
{
	//movss xmm? dword ptr Value
	pBlock->StreamWrite(4, 0xF3, 0x0F, 0x10, 0x00 | (m_nRegister << 3) | 0x05);
	pBlock->StreamWriteWord((uint32)((uint8*)(pValue) - (uint8*)0));

	//shufps xmm?, xmm?, 0x00
	pBlock->StreamWrite(4, 0x0F, 0xC6, 0xC0 | (m_nRegister << 3) | (m_nRegister), 0x00);

	m_nRegister++;
}

void CVUF128::CSSEFactory::PushImm(CCacheBlock* pBlock, float nValue)
{
	ReserveStack(m_pBlock, 0x10);

	//mov [esp], nValue
	pBlock->StreamWrite(3, 0xC7, 0x04, 0x24);
	pBlock->StreamWriteWord(*(uint32*)&nValue);

	//movss xmm?, [esp]
	pBlock->StreamWrite(5, 0xF3, 0x0F, 0x10, 0x00 | (m_nRegister << 3) | 0x04, 0x24);

	//shufps xmm?, xmm?, 0x00
	pBlock->StreamWrite(4, 0x0F, 0xC6, 0xC0 | (m_nRegister << 3) | (m_nRegister), 0x00);

	m_nRegister++;
}

void CVUF128::CSSEFactory::PushTop(CCacheBlock* pBlock)
{
	m_nRegister--;

	//movaps xmm(? + 1), xmm?
	pBlock->StreamWrite(3, 0x0F, 0x28, 0xC0 | ((m_nRegister + 1) << 3) | m_nRegister);

	m_nRegister += 2;
}

void CVUF128::CSSEFactory::Pull(CCacheBlock* pBlock, uint32* pElement0, uint32* pElement1, uint32* pElement2, uint32* pElement3)
{
	unsigned int i;
	void* pElement[4];
	uint8 nShuffle[4] = { 0x00, 0xE5, 0xEA, 0xFF };

	pElement[0] = pElement0;
	pElement[1] = pElement1;
	pElement[2] = pElement2;
	pElement[3] = pElement3;

	m_nRegister--;

	if(pElement0 && pElement1 && pElement2 && pElement3)
	{
		//All elements are non-null
		if((pElement1 == pElement0 + 1) && (pElement2 == pElement1 + 1) && (pElement3 == pElement2 + 1))
		{
			//Sequencial

			//movaps pElement0, xmm?
			pBlock->StreamWrite(3, 0x0F, 0x29, 0x00 | (m_nRegister << 3) | 0x05);
			pBlock->StreamWriteWord((uint32)((uint8*)(pElement0) - (uint8*)0));
			return;
		}
	}

	for(i = 0; i < 4; i++)
	{
		if(pElement[i] != NULL)
		{
			if(i != 0)
			{
				//shufps xmm?, xmm?, 0xE5
				pBlock->StreamWrite(4, 0x0F, 0xC6, 0xC0 | (m_nRegister << 3) | (m_nRegister), nShuffle[i]);
			}

			//movss [pElement], xmm?
			pBlock->StreamWrite(4, 0xF3, 0x0F, 0x11, 0x00 | (m_nRegister << 3) | 0x05);
			pBlock->StreamWriteWord((uint32)((uint8*)(pElement[i]) - (uint8*)0));
		}
	}
}

void CVUF128::CSSEFactory::Add(CCacheBlock* pBlock)
{
	m_nRegister -= 2;

	//addps xmm?, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x58, 0xC0 | ((m_nRegister + 0) << 3) | (m_nRegister + 1));

	m_nRegister++;
}

void CVUF128::CSSEFactory::Max(CCacheBlock* pBlock)
{
	m_nRegister -= 2;

	//maxps xmm?, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x5F, 0xC0 | ((m_nRegister + 0) << 3) | (m_nRegister + 1));

	m_nRegister++;
}

void CVUF128::CSSEFactory::Min(CCacheBlock* pBlock)
{
	m_nRegister -= 2;

	//minps xmm?, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x5D, 0xC0 | ((m_nRegister + 0) << 3) | (m_nRegister + 1));

	m_nRegister++;
}

void CVUF128::CSSEFactory::Mul(CCacheBlock* pBlock)
{
	m_nRegister -= 2;

	//mulps xmm?, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x59, 0xC0 | ((m_nRegister + 0) << 3) | (m_nRegister + 1));

	m_nRegister++;
}

void CVUF128::CSSEFactory::Sub(CCacheBlock* pBlock)
{
	m_nRegister -= 2;

	//subps xmm0, xmm1
	pBlock->StreamWrite(3, 0x0F, 0x5C, 0xC0 | ((m_nRegister + 0) << 3) | (m_nRegister + 1));

	m_nRegister++;
}

void CVUF128::CSSEFactory::Truncate(CCacheBlock* pBlock)
{
	m_nRegister--;

	RequireEMMS();

	//cvttps2pi mm0, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x2C, 0xC0 | (0x00 << 3) | (m_nRegister));

	//movhlps xmm?, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x12, 0xC0 | (m_nRegister << 3) | (m_nRegister));

	//cvttps2ps mm1, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x2C, 0xC0 | (0x01 << 3) | (m_nRegister));

	//cvtpi2ps xmm?, mm1
	pBlock->StreamWrite(3, 0x0F, 0x2A, 0xC0 | (m_nRegister << 3) | (0x01));

	//shufps xmm?, xmm?, 0x10
	pBlock->StreamWrite(4, 0x0F, 0xC6, 0xC0 | (m_nRegister << 3) | (m_nRegister), 0x40);

	//cvtpi2ps xmm?, mm0
	pBlock->StreamWrite(3, 0x0F, 0x2A, 0xC0 | (m_nRegister << 3) | (0x00));

	m_nRegister++;
}

void CVUF128::CSSEFactory::IsZero(CCacheBlock* pBlock)
{
	m_nRegister--;

	ReserveStack(pBlock, 0x10);
	RequireEMMS();

	//movups [esp], xmm?
	pBlock->StreamWrite(4, 0x0F, 0x11, 0x00 | (m_nRegister << 3) | 0x04, 0x24);

	//pandn mm0, mm0 (mm0 <- 0)
	pBlock->StreamWrite(3, 0x0F, 0xDF, 0xC0 | (0x00 << 3) | (0x00));

	//pandn mm1, mm1 (mm1 <- 0)
	pBlock->StreamWrite(3, 0x0F, 0xDF, 0xC0 | (0x01 << 3) | (0x00));

	//pcmpeqd mm0, [esp + 0x00]
	pBlock->StreamWrite(4, 0x0F, 0x76, 0x00 | (0x00 << 3) | 0x04, 0x24);

	//pcmpeqd mm1, [esp + 0x08]
	pBlock->StreamWrite(5, 0x0F, 0x76, 0x40 | (0x01 << 3) | 0x04, 0x24, 0x08);

	//movq [esp], mm0
	pBlock->StreamWrite(4, 0x0F, 0x7F, 0x00 | (0x00 << 3) | 0x04, 0x24);

	//movq [esp + 0x08], mm1
	pBlock->StreamWrite(5, 0x0F, 0x7F, 0x40 | (0x01 << 3) | 0x04, 0x24, 0x08);

	//movups xmm?, [esp]
	pBlock->StreamWrite(4, 0x0F, 0x10, 0x00 | (m_nRegister << 3) | 0x04, 0x24);

	m_nRegister++;
}

void CVUF128::CSSEFactory::IsNegative(CCacheBlock* pBlock)
{
	m_nRegister--;

	ReserveStack(pBlock, 0x10);
	RequireEMMS();

	//movups [esp], xmm?
	pBlock->StreamWrite(4, 0x0F, 0x11, 0x00 | (m_nRegister << 3) | 0x04, 0x24);

	//pcmpeqd mm0, mm0 (mm0 <- 0xFFFFFFFF)
	pBlock->StreamWrite(3, 0x0F, 0x76, 0xC0 | (0x00 << 3) | (0x00));

	//pcmpeqd mm1, mm1 (mm1 <- 0xFFFFFFFF)
	pBlock->StreamWrite(3, 0x0F, 0x76, 0xC0 | (0x01 << 3) | (0x01));

	//pslld mm0, 31 (mm0 <- 0x80000000)
	pBlock->StreamWrite(4, 0x0F, 0x72, 0xF0 | 0x00, 0x1F);

	//pslld mm1, 31 (mm1 <- 0x80000000)
	pBlock->StreamWrite(4, 0x0F, 0x72, 0xF0 | 0x01, 0x1F);

	//pand mm0, [esp + 0x00]
	pBlock->StreamWrite(4, 0x0F, 0xDB, 0x00 | (0x00 << 3) | 0x04, 0x24);

	//pand mm1, [esp + 0x08]
	pBlock->StreamWrite(5, 0x0F, 0xDB, 0x40 | (0x01 << 3) | 0x04, 0x24, 0x08);

	//movq [esp], mm0
	pBlock->StreamWrite(4, 0x0F, 0x7F, 0x00 | (0x00 << 3) | 0x04, 0x24);

	//movq [esp + 0x08], mm1
	pBlock->StreamWrite(5, 0x0F, 0x7F, 0x40 | (0x01 << 3) | 0x04, 0x24, 0x08);

	//movups xmm?, [esp]
	pBlock->StreamWrite(4, 0x0F, 0x10, 0x00 | (m_nRegister << 3) | 0x04, 0x24);

	m_nRegister++;
}

void CVUF128::CSSEFactory::ToWordTruncate(CCacheBlock* pBlock)
{
	m_nRegister--;

	ReserveStack(pBlock, 0x10);
	RequireEMMS();

	//cvttps2pi mm0, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x2C, 0xC0 | (0x00 << 3) | (m_nRegister));

	//movhlps xmm?, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x12, 0xC0 | (m_nRegister << 3) | (m_nRegister));

	//cvttps2pi mm1, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x2C, 0xC0 | (0x01 << 3) | (m_nRegister));

	//movq [esp + 0x00], mm0
	pBlock->StreamWrite(4, 0x0F, 0x7F, 0x00 | (0x00 << 3) | 0x04, 0x24);

	//movq [esp + 0x04], mm1
	pBlock->StreamWrite(5, 0x0F, 0x7F, 0x40 | (0x01 << 3) | 0x04, 0x24, 0x08);

	//movups xmm0, [esp]
	pBlock->StreamWrite(4, 0x0F, 0x10, 0x00 | (m_nRegister << 3) | 0x04, 0x24);

	m_nRegister++;
}

void CVUF128::CSSEFactory::ToSingle(CCacheBlock* pBlock)
{
	m_nRegister -= 1;

	ReserveStack(pBlock, 0x10);
	RequireEMMS();

	//movups [esp], xmm?
	pBlock->StreamWrite(4, 0x0F, 0x11, 0x00 | (m_nRegister << 3) | 0x04, 0x24);

	//movq mm0, [esp + 0x0]
	pBlock->StreamWrite(4, 0x0F, 0x6F, 0x00 | (0x00 << 3) | 0x04, 0x24);

	//movq mm1, [esp + 0x4]
	pBlock->StreamWrite(5, 0x0F, 0x6F, 0x40 | (0x01 << 3) | 0x04, 0x24, 0x08);

	//cvtpi2ps xmm0, mm1
	pBlock->StreamWrite(3, 0x0F, 0x2A, 0xC0 | (m_nRegister << 3) | 0x01);

	//movlhps xmm0, xmm0
	pBlock->StreamWrite(3, 0x0F, 0x16, 0xC0 | (m_nRegister << 3) | (m_nRegister));

	//cvtpi2ps xmm0, mm0
	pBlock->StreamWrite(3, 0x0F, 0x2A, 0xC0 | (m_nRegister << 3) | 0x00);

	m_nRegister++;
}

void CVUF128::CSSEFactory::ReserveStack(CCacheBlock* pBlock, uint8 nAmount)
{
	if(nAmount > m_nStackAlloc)
	{
		//sub esp, 0x10
		pBlock->StreamWrite(3, 0x83, 0xEC, nAmount - m_nStackAlloc);
	}

	m_nStackAlloc = nAmount;
}

void CVUF128::CSSEFactory::RequireEMMS()
{
	m_nNeedsEMMS = true;
}

/////////////////////////////////////////////
//SSE2 Factory Implementation
/////////////////////////////////////////////

CVUF128::CSSE2Factory::CSSE2Factory()
{
	m_nRegister = 0;
}

CVUF128::CSSE2Factory::~CSSE2Factory()
{

}

void CVUF128::CSSE2Factory::IsZero(CCacheBlock* pBlock)
{
	m_nRegister--;

	//pandn xmm? + 1, xmm? + 1 (xmm? + 1 <- 0)
	pBlock->StreamWrite(4, 0x66, 0x0F, 0xDF, 0xC0 | ((m_nRegister + 1) << 3) | (m_nRegister + 1));

	//pcmpeqd xmm?, xmm? + 1
	pBlock->StreamWrite(4, 0x66, 0x0F, 0x76, 0xC0 | (m_nRegister << 3) | (m_nRegister + 1));

	m_nRegister++;
}

void CVUF128::CSSE2Factory::IsNegative(CCacheBlock* pBlock)
{
	m_nRegister--;

	//pcmpeqd xmm? + 1, xmm? + 1
	pBlock->StreamWrite(4, 0x66, 0x0F, 0x76, 0xC0 | ((m_nRegister + 1) << 3) | (m_nRegister + 1));

	//pslld xmm? + 1, 31
	pBlock->StreamWrite(5, 0x66, 0x0F, 0x72, 0xF0 | (m_nRegister + 1), 0x1F);

	//pand xmm?, xmm? + 1
	pBlock->StreamWrite(4, 0x66, 0x0F, 0xDB, 0xC0 | (m_nRegister << 3) | (m_nRegister + 1));

	m_nRegister++;
}

void CVUF128::CSSE2Factory::ToWordTruncate(CCacheBlock* pBlock)
{
	m_nRegister--;

	RequireEMMS();

	//cvttps2pi mm0, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x2C, 0xC0 | (0x00 << 3) | (m_nRegister));

	//movhlps xmm?, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x12, 0xC0 | (m_nRegister << 3) | (m_nRegister));

	//cvttps2pi mm1, xmm?
	pBlock->StreamWrite(3, 0x0F, 0x2C, 0xC0 | (0x01 << 3) | (m_nRegister));

	//movq2dq xmm?, mm0
	pBlock->StreamWrite(4, 0xF3, 0x0F, 0xD6, 0xC0 | (m_nRegister << 3) | (0x00));

	//movq2dq xmm? + 1, mm1
	pBlock->StreamWrite(4, 0xF3, 0x0F, 0xD6, 0xC0 | ((m_nRegister + 1) << 3) | (0x01));

	//movlhps xmm?, xmm? + 1
	pBlock->StreamWrite(3, 0x0F, 0x16, 0xC0 | (m_nRegister << 3) | (m_nRegister + 1));

	m_nRegister++;
}

void CVUF128::CSSE2Factory::ToSingle(CCacheBlock* pBlock)
{
	m_nRegister -= 1;

	RequireEMMS();

	//movdq2q mm0, xmm0
	pBlock->StreamWrite(4, 0xF2, 0x0F, 0xD6, 0xC0 | (0x00 << 3) | (m_nRegister));

	//movhlps xmm0, xmm0
	pBlock->StreamWrite(3, 0x0F, 0x12, 0xC0 | (m_nRegister << 3) | (m_nRegister));

	//movdq2q mm1, xmm0
	pBlock->StreamWrite(4, 0xF2, 0x0F, 0xD6, 0xC0 | (0x01 << 3) | (m_nRegister));

	//cvtpi2ps xmm0, mm1
	pBlock->StreamWrite(3, 0x0F, 0x2A, 0xC0 | (m_nRegister << 3) | 0x01);

	//movlhps xmm0, xmm0
	pBlock->StreamWrite(3, 0x0F, 0x16, 0xC0 | (m_nRegister << 3) | (m_nRegister));

	//cvtpi2ps xmm0, mm0
	pBlock->StreamWrite(3, 0x0F, 0x2A, 0xC0 | (m_nRegister << 3) | 0x00);

	m_nRegister++;
}
