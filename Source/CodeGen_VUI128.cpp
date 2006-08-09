#include <assert.h>
#include "CodeGen_VUI128.h"
#include "PtrMacro.h"
#ifdef AMD64
#include "amd64/CPUID.h"
#endif

using namespace CodeGen;

CCacheBlock*			CVUI128::m_pBlock = NULL;
CArrayStack<uint32>		CVUI128::m_OpStack;
CVUI128::CFactory*		CVUI128::m_pFactory = CreateFactory();

void CVUI128::Begin(CCacheBlock* pBlock)
{
	m_pBlock = pBlock;
	m_OpStack.Reset();
	m_pFactory->Begin(pBlock);
}

void CVUI128::End()
{
	m_pFactory->End();
}

CVUI128::CFactory* CVUI128::CreateFactory()
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

	//Force SSE
	//return new CSSEFactory();

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

void CVUI128::Push(uint128* pValue)
{
	m_OpStack.Push((uint32)((uint8*)(pValue) - (uint8*)0));
	m_OpStack.Push(VARIABLE);
}

void CVUI128::Pull(uint128* pValue)
{
	if(m_OpStack.GetAt(0) == STACK)
	{
		int i;
		uint32 nAddress;

		nAddress = (uint32)((uint8*)(pValue) - (uint8*)0);

		for(i = 0; i < 4; i++)
		{
			//pop [pValue + i * 4]
			m_pBlock->StreamWrite(2, 0x8F, 0x00 | (0x00 << 3) | (0x05));
			m_pBlock->StreamWriteWord(nAddress + (i * 4));
		}

		m_OpStack.Pull();
		m_OpStack.Pull();
	}
	else
	{
		m_pFactory->Pull(pValue);
	}
}

void CVUI128::AddH()
{
	m_pFactory->AddH();
}

void CVUI128::AddWSS()
{
	int i;
	uint32 nVariable1, nVariable2;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	for(i = 3; i >= 0; i--)
	{
		//mov eax, dword ptr[nVariable1 + i * 4];
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (0x00 << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable1 + (i * 4));

		//add eax, dword ptr[nVariable2 + i * 4];
		m_pBlock->StreamWrite(2, 0x03, 0x00 | (0x00 << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2 + (i * 4));

		//jno +0x10
		m_pBlock->StreamWrite(2, 0x71, 0x10);

		//mov ecx, 0x7FFFFFFF
		m_pBlock->StreamWrite(1, 0xB9);
		m_pBlock->StreamWriteWord(0x7FFFFFFF);

		//mov ebx, 0x80000000
		m_pBlock->StreamWrite(1, 0xBB);
		m_pBlock->StreamWriteWord(0x80000000);

		//cmovs eax, ecx
		m_pBlock->StreamWrite(3, 0x0F, 0x48, 0xC1);

		//cmovs eax, ebx
		m_pBlock->StreamWrite(3, 0x0F, 0x49, 0xC3);

		//push eax
		m_pBlock->StreamWrite(1, 0x50);
	}

	m_OpStack.Push(0x10);
	m_OpStack.Push(STACK);
}

void CVUI128::AddWUS()
{
	int i;
	uint32 nVariable1, nVariable2;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	for(i = 3; i >= 0; i--)
	{
		//mov eax, dword ptr[nVariable1 + i * 4];
		m_pBlock->StreamWrite(2, 0x8B, 0x00 | (0x00 << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable1 + (i * 4));

		//add eax, dword ptr[nVariable2 + i * 4];
		m_pBlock->StreamWrite(2, 0x03, 0x00 | (0x00 << 3) | (0x05));
		m_pBlock->StreamWriteWord(nVariable2 + (i * 4));

		//jnc +0x04
		m_pBlock->StreamWrite(2, 0x73, 0x04);

		//push 0xFFFFFFFF
		m_pBlock->StreamWrite(2, 0x6A, 0xFF);

		//jmp +0x01
		m_pBlock->StreamWrite(2, 0xEB, 0x01);

		//push eax
		m_pBlock->StreamWrite(1, 0x50);
	}

	m_OpStack.Push(0x10);
	m_OpStack.Push(STACK);
}

void CVUI128::And()
{
	m_pFactory->And();
}

void CVUI128::CmpEqW()
{
	m_pFactory->CmpEqW();
}

void CVUI128::CmpGtH()
{
	m_pFactory->CmpGtH();
}

void CVUI128::MaxH()
{
	m_pFactory->MaxH();
}

void CVUI128::MinH()
{
	m_pFactory->MinH();
}

void CVUI128::Not()
{
	m_pFactory->Not();
}

void CVUI128::Or()
{
	m_pFactory->Or();
}

void CVUI128::PackHB()
{
	m_pFactory->PackHB();
}

void CVUI128::PackWH()
{
	uint32 nVariable1, nVariable2;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	//add esp, 0x10
	m_pBlock->StreamWrite(3, 0x83, 0xEC, 0x10);

//		004B7DA0 66 8B 41 04      mov         ax,word ptr [ecx+4] 
//		004B7DA4 66 8B 5A 04      mov         bx,word ptr [edx+4] 
//		004B7DA8 66 89 5C 24 02   mov         word ptr [esp+2],bx 
//		004B7DAD 66 89 44 24 0A   mov         word ptr [esp+0Ah],ax 

	//mov ecx, nVariable1
	m_pBlock->StreamWrite(1, 0xB9);
	m_pBlock->StreamWriteWord(nVariable1);

	//mov edx, nVariable2
	m_pBlock->StreamWrite(1, 0xBA);
	m_pBlock->StreamWriteWord(nVariable2);

	for(int i = 0; i < 4; i++)
	{
		//mov ax, [ecx + (i * 4)]
		m_pBlock->StreamWrite(4, 0x66, 0x8B, 0x41, (i * 4));

		//mov bx, [edx + (i * 4)]
		m_pBlock->StreamWrite(4, 0x66, 0x8B, 0x5A, (i * 4));

		//mov [esp + (i * 2) + 0x00], bx
		m_pBlock->StreamWrite(5, 0x66, 0x89, 0x5C, 0x24, (i * 2) + 0x00);

		//mov [esp + (i * 2) + 0x08], ax
		m_pBlock->StreamWrite(5, 0x66, 0x89, 0x44, 0x24, (i * 2) + 0x08);
	}

	m_OpStack.Push(0x10);
	m_OpStack.Push(STACK);
}

void CVUI128::SubB()
{
	m_pFactory->SubB();
}

void CVUI128::SubW()
{
	m_pFactory->SubW();
}

void CVUI128::SllH(unsigned int nAmount)
{
	m_pFactory->SllH(nAmount);
}

void CVUI128::SrlH(unsigned int nAmount)
{
	m_pFactory->SrlH(nAmount);
}

void CVUI128::SraH(unsigned int nAmount)
{
	m_pFactory->SraH(nAmount);
}

void CVUI128::SrlVQw(uint32* pSA)
{
	uint32 nVariable1, nVariable2;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	//Copy both registers on the stack
	//-----------------------------

	//add esp, 0x30
	m_pBlock->StreamWrite(3, 0x83, 0xEC, 0x30);

	//lea edi, esp
	m_pBlock->StreamWrite(2, 0x8B, 0xFC);

	//mov esi, nVariable2
	m_pBlock->StreamWrite(1, 0xBE);
	m_pBlock->StreamWriteWord(nVariable2);

	//mov ecx, 0x10
	m_pBlock->StreamWrite(5, 0xB9, 0x10, 0x00, 0x00, 0x00);

	//rep movs
	m_pBlock->StreamWrite(2, 0xF3, 0xA4);

	//mov esi, nVariable1
	m_pBlock->StreamWrite(1, 0xBE);
	m_pBlock->StreamWriteWord(nVariable1);

	//mov ecx, 0x10
	m_pBlock->StreamWrite(5, 0xB9, 0x10, 0x00, 0x00, 0x00);

	//rep movs
	m_pBlock->StreamWrite(2, 0xF3, 0xA4);

	//Setup SA
	//-----------------------------

	//mov eax, dword ptr[pSA]
	m_pBlock->StreamWrite(2, 0x8B, 0x05);
	m_pBlock->StreamWriteWord((uint32)((uint8*)pSA - (uint8*)0));

	//and eax, 0x7F
	m_pBlock->StreamWrite(3, 0x83, 0xE0, 0x7F);

	//shr eax, 3
	m_pBlock->StreamWrite(3, 0xC1, 0xE8, 0x03);

	//Setup ESI
	//-----------------------------

	//lea esi, [esp + eax]
	m_pBlock->StreamWrite(3, 0x8D, 0x34, 0x04);

	//Setup ECX
	//-----------------------------

	//mov ecx, 0x10
	m_pBlock->StreamWrite(5, 0xB9, 0x10, 0x00, 0x00, 0x00);

	//Generate result
	//-----------------------------
	
	//rep movs
	m_pBlock->StreamWrite(2, 0xF3, 0xA4);

	//Free Stack
	//-----------------------------

	//add esp, 0x20
	m_pBlock->StreamWrite(3, 0x83, 0xC4, 0x20);

	m_OpStack.Push(0x10);
	m_OpStack.Push(STACK);
}

void CVUI128::UnpackLowerBH()
{
	m_pFactory->UnpackLowerBH();
}

void CVUI128::UnpackUpperBH()
{
	m_pFactory->UnpackUpperBH();
}

void CVUI128::UnpackLowerHW()
{
	m_pFactory->UnpackLowerHW();
}

void CVUI128::UnpackLowerWD()
{
	m_pFactory->UnpackLowerWD();
}

void CVUI128::UnpackUpperWD()
{
	m_pFactory->UnpackUpperWD();
}

void CVUI128::Xor()
{
	m_pFactory->Xor();
}

/////////////////////////////////////////////
//Factory Implementation
/////////////////////////////////////////////

CVUI128::CFactory::~CFactory()
{

}

/////////////////////////////////////////////
//MMX Factory Implementation
/////////////////////////////////////////////

void CVUI128::CSSEFactory::Begin(CCacheBlock* pBlock)
{
	unsigned int i;

	m_nRequiresEMMS = false;

	m_pBlock = pBlock;
	for(i = 0; i < MAX_REGISTER; i++)
	{
		m_nRegisterAllocated[i] = false;
	}
}

void CVUI128::CSSEFactory::End()
{
	if(m_nRequiresEMMS)
	{
		//emms
		m_pBlock->StreamWrite(2, 0x0F, 0x77);
	}
}

unsigned int CVUI128::CSSEFactory::AllocateRegister()
{
	unsigned int i;

	for(i = 0; i < MAX_REGISTER; i++)
	{
		if(m_nRegisterAllocated[i] == false)
		{
			m_nRegisterAllocated[i] = true;
			return i * 2;
		}
	}

	assert(0);
	return 0;
}

void CVUI128::CSSEFactory::FreeRegister(unsigned int nRegister)
{
	m_nRegisterAllocated[nRegister / 2] = false;
}

void CVUI128::CSSEFactory::RequireEMMS()
{
	m_nRequiresEMMS = true;
}

void CVUI128::CSSEFactory::BeginTwoOperands(uint32* nDstRegister, uint32* nSrcVariable)
{
	uint32 nVariable1, nVariable2, nRegister;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	RequireEMMS();

	nRegister = AllocateRegister();

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	//movq mm? + 0, [nVariable1 + 0]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nRegister + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable1 + 0);

	//movq mm? + 1, [nVariable1 + 8]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nRegister + 1) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable1 + 8);

	(*nDstRegister) = nRegister;
	(*nSrcVariable) = nVariable2;
}

void CVUI128::CSSEFactory::EndTwoOperands(uint32 nRegister)
{
	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::LoadVariableInRegister(uint32 nVariable, unsigned int nRegister)
{
	//movq mm? + 0, [nVariable1 + 0]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nRegister + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable + 0);

	//movq mm? + 1, [nVariable1 + 8]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nRegister + 1) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable + 8);
}

void CVUI128::CSSEFactory::Pull(uint128* pValue)
{
	uint32 nRegister, nVariable;

	assert(m_OpStack.GetAt(0) == REGISTER);

	RequireEMMS();

	m_OpStack.Pull();
	nRegister = m_OpStack.Pull();
	FreeRegister(nRegister);

	nVariable = (uint32)((uint8*)(pValue) - (uint8*)0);

	//movq [pValue + 0], mm?
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0x00 | ((nRegister + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable + 0);

	//movq [pValue + 4], mm? + 1
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0x00 | ((nRegister + 1) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable + 8);
}

void CVUI128::CSSEFactory::AddH()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//paddw mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0xFD, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//paddw mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0xFD, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::And()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pand mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0xDB, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//pand mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0xDB, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::CmpEqW()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pcmpeqd mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0x66, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//pcmpeqd mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0x66, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::CmpGtH()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pcmpgtw mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0x65, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//pcmpgtw mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0x65, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::MaxH()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pmaxsw mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0xEE, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//pmaxsw mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0xEE, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::MinH()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pminsw mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0xEA, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//pminsw mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0xEA, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::Not()
{
	uint32 nRegister1, nRegister2;

	assert(m_OpStack.GetAt(0) == REGISTER);

	RequireEMMS();

	nRegister2 = AllocateRegister();

	m_OpStack.Pull();
	nRegister1 = m_OpStack.Pull();

	//pcmpeqd mm?, mm?
	m_pBlock->StreamWrite(3, 0x0F, 0x76, 0xC0 | (nRegister2 << 3) | (nRegister2));

	//pxor mm? + 0, mm?
	m_pBlock->StreamWrite(3, 0x0F, 0xEF, 0xC0 | ((nRegister1 + 0) << 3) | (nRegister2));

	//pxor mm? + 1, mm?
	m_pBlock->StreamWrite(3, 0x0F, 0xEF, 0xC0 | ((nRegister1 + 1) << 3) | (nRegister2));

	FreeRegister(nRegister2);

	m_OpStack.Push(nRegister1);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::Or()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//por mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0xEB, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//por mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0xEB, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::PackHB()
{
	uint32 nVariable1, nVariable2;
	uint32 nRegister1, nRegister2, nMask;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	RequireEMMS();

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	nRegister1	= AllocateRegister();
	nRegister2	= AllocateRegister();
	nMask		= AllocateRegister();

	LoadVariableInRegister(nVariable2, nRegister1);
	LoadVariableInRegister(nVariable1, nRegister2);

	//Load 0x00FF x 8 in a temporary register

	//pcmpeqd nMask, nMask
	m_pBlock->StreamWrite(3, 0x0F, 0x76, 0xC0 | (nMask << 3) | (nMask));

	//psrlw nMask, nAmount
	m_pBlock->StreamWrite(4, 0x0F, 0x71, 0xD0 | (nMask), 0x08);

	//pand mm0, nMask
	m_pBlock->StreamWrite(3, 0x0F, 0xDB, 0xC0 | ((nRegister1 + 0) << 3) | (nMask));

	//pand mm1, nMask
	m_pBlock->StreamWrite(3, 0x0F, 0xDB, 0xC0 | ((nRegister1 + 1) << 3) | (nMask));

	//pand mm2, nMask
	m_pBlock->StreamWrite(3, 0x0F, 0xDB, 0xC0 | ((nRegister2 + 0) << 3) | (nMask));

	//pand mm3, nMask
	m_pBlock->StreamWrite(3, 0x0F, 0xDB, 0xC0 | ((nRegister2 + 1) << 3) | (nMask));

	//packuswb mm0, mm1
	m_pBlock->StreamWrite(3, 0x0F, 0x67, 0xC0 | ((nRegister1 + 0) << 3) | (nRegister1 + 1));

	//packuswb mm2, mm3
	m_pBlock->StreamWrite(3, 0x0F, 0x67, 0xC0 | ((nRegister2 + 0) << 3) | (nRegister2 + 1));

	//movq mm1, mm2
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nRegister2 + 0) << 3) | (nRegister1 + 1));

	FreeRegister(nRegister2);
	FreeRegister(nMask);

	m_OpStack.Push(nRegister1);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::SubB()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//psubb mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0xF8, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//psubb mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0xF8, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::SubW()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//psubd mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0xFA, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//psubd mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0xFA, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSEFactory::SllH(unsigned int nAmount)
{
	uint32 nVariable, nRegister;

	assert(m_OpStack.GetAt(0) == VARIABLE);

	RequireEMMS();

	nRegister = AllocateRegister();

	m_OpStack.Pull();
	nVariable = m_OpStack.Pull();

	LoadVariableInRegister(nVariable, nRegister);

	//psllw mm? + 0, nAmount
	m_pBlock->StreamWrite(4, 0x0F, 0x71, 0xF0 | (nRegister + 0), nAmount);

	//psllw mm? + 1, nAmount
	m_pBlock->StreamWrite(4, 0x0F, 0x71, 0xF0 | (nRegister + 1), nAmount);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::SrlH(unsigned int nAmount)
{
	uint32 nVariable, nRegister;

	assert(m_OpStack.GetAt(0) == VARIABLE);

	nRegister = AllocateRegister();

	RequireEMMS();

	m_OpStack.Pull();
	nVariable = m_OpStack.Pull();

	LoadVariableInRegister(nVariable, nRegister);

	//psrlw mm? + 0, nAmount
	m_pBlock->StreamWrite(4, 0x0F, 0x71, 0xD0 | (nRegister + 0), nAmount);

	//psrlw mm? + 1, nAmount
	m_pBlock->StreamWrite(4, 0x0F, 0x71, 0xD0 | (nRegister + 1), nAmount);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::SraH(unsigned int nAmount)
{
	uint32 nVariable, nRegister;

	assert(m_OpStack.GetAt(0) == VARIABLE);

	RequireEMMS();

	nRegister = AllocateRegister();

	m_OpStack.Pull();
	nVariable = m_OpStack.Pull();

	LoadVariableInRegister(nVariable, nRegister);

	//psraw mm? + 0, nAmount
	m_pBlock->StreamWrite(4, 0x0F, 0x71, 0xE0 | (nRegister + 0), nAmount);

	//psraw mm? + 1, nAmount
	m_pBlock->StreamWrite(4, 0x0F, 0x71, 0xE0 | (nRegister + 1), nAmount);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::UnpackLowerBH()
{
	uint32 nVariable1, nVariable2;
	uint32 nRegister1, nRegister2, nTemp;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	RequireEMMS();

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	nRegister1	= AllocateRegister();
	nRegister2	= AllocateRegister();
	nTemp		= AllocateRegister();

	LoadVariableInRegister(nVariable1, nRegister1);
	LoadVariableInRegister(nVariable2, nRegister2);

	//mm0, mm1 - R0
	//mm2, mm3 - R1

	//mm0 contains Aw[1] | Aw[0]
	//mm1 contains Aw[3] | Aw[2]
	//mm2 contains Bw[1] | Bw[0]
	//mm3 contains Bw[3] | Bw[2]

	//Save mm0, mm2
	//---------------------------------

	//movq mm4, mm0
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nRegister1 + 0) << 3) | (nTemp + 0));

	//movq mm5, mm2
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nRegister2 + 0) << 3) | (nTemp + 1));

	//Compute : 
	//Aw[2] | Aw[0]
	//Aw[3] | Aw[1]
	//Bw[2] | Bw[0]
	//Bw[3] | Bw[1]
	//---------------------------------

	//punpckldq mm4, mm1
	//mm4 -> Aw[2] | Aw[0]
	m_pBlock->StreamWrite(3, 0x0F, 0x62, 0xC0 | ((nTemp + 0) << 3) | (nRegister1 + 1));

	//punpckhdq mm0, mm1
	//mm0 -> Aw[3] | Aw[1]
	m_pBlock->StreamWrite(3, 0x0F, 0x6A, 0xC0 | ((nRegister1 + 0) << 3) | (nRegister1 + 1));

	//punpckldq mm5, mm3
	//mm5 -> Bw[2] | Bw[0]
	m_pBlock->StreamWrite(3, 0x0F, 0x62, 0xC0 | ((nTemp + 1) << 3) | (nRegister2 + 1));

	//punpckhdq mm2, mm3
	//mm2 -> Bw[3] | Bw[1]
	m_pBlock->StreamWrite(3, 0x0F, 0x6A, 0xC0 | ((nRegister2 + 0) << 3) | (nRegister2 + 1));

	//Actual operation
	//---------------------------------

	//punpcklbw mm5, mm4
	m_pBlock->StreamWrite(3, 0x0F, 0x60, 0xC0 | ((nTemp + 1) << 3) | (nTemp + 0));

	//punpcklbw mm2, mm0
	m_pBlock->StreamWrite(3, 0x0F, 0x60, 0xC0 | ((nRegister2 + 0) << 3) | (nRegister1 + 0));

	//Store result
	//---------------------------------

	//movq mm0, mm5
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nTemp + 1) << 3) | (nRegister1 + 0));

	//movq mm1, mm2
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nRegister2 + 0) << 3) | (nRegister1 + 1));

	FreeRegister(nRegister2);
	FreeRegister(nTemp);

	m_OpStack.Push(nRegister1);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::UnpackUpperBH()
{
	uint32 nVariable1, nVariable2;
	uint32 nRegister1, nRegister2, nTemp;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	RequireEMMS();

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	nRegister1	= AllocateRegister();
	nRegister2	= AllocateRegister();
	nTemp		= AllocateRegister();

	LoadVariableInRegister(nVariable1, nRegister1);
	LoadVariableInRegister(nVariable2, nRegister2);

	//mm0, mm1 - R0
	//mm2, mm3 - R1

	//mm0 contains Aw[1] | Aw[0]
	//mm1 contains Aw[3] | Aw[2]
	//mm2 contains Bw[1] | Bw[0]
	//mm3 contains Bw[3] | Bw[2]

	//Save mm0, mm2
	//---------------------------------

	//movq mm4, mm0
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nRegister1 + 0) << 3) | (nTemp + 0));

	//movq mm5, mm2
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nRegister2 + 0) << 3) | (nTemp + 1));

	//Compute : 
	//Aw[2] | Aw[0]
	//Aw[3] | Aw[1]
	//Bw[2] | Bw[0]
	//Bw[3] | Bw[1]
	//---------------------------------

	//punpckldq mm4, mm1
	//mm4 -> Aw[2] | Aw[0]
	m_pBlock->StreamWrite(3, 0x0F, 0x62, 0xC0 | ((nTemp + 0) << 3) | (nRegister1 + 1));

	//punpckhdq mm0, mm1
	//mm0 -> Aw[3] | Aw[1]
	m_pBlock->StreamWrite(3, 0x0F, 0x6A, 0xC0 | ((nRegister1 + 0) << 3) | (nRegister1 + 1));

	//punpckldq mm5, mm3
	//mm5 -> Bw[2] | Bw[0]
	m_pBlock->StreamWrite(3, 0x0F, 0x62, 0xC0 | ((nTemp + 1) << 3) | (nRegister2 + 1));

	//punpckhdq mm2, mm3
	//mm2 -> Bw[3] | Bw[1]
	m_pBlock->StreamWrite(3, 0x0F, 0x6A, 0xC0 | ((nRegister2 + 0) << 3) | (nRegister2 + 1));

	//Actual operation
	//---------------------------------

	//punpckhbw mm5, mm4
	m_pBlock->StreamWrite(3, 0x0F, 0x68, 0xC0 | ((nTemp + 1) << 3) | (nTemp + 0));

	//punpckhbw mm2, mm0
	m_pBlock->StreamWrite(3, 0x0F, 0x68, 0xC0 | ((nRegister2 + 0) << 3) | (nRegister1 + 0));

	//Store result
	//---------------------------------

	//movq mm0, mm5
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nTemp + 1) << 3) | (nRegister1 + 0));

	//movq mm1, mm2
	m_pBlock->StreamWrite(3, 0x0F, 0x7F, 0xC0 | ((nRegister2 + 0) << 3) | (nRegister1 + 1));

	FreeRegister(nRegister2);
	FreeRegister(nTemp);

	m_OpStack.Push(nRegister1);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::UnpackLowerHW()
{
	//TODO: This hasn't been tested properly

	uint32 nVariable1, nVariable2;
	uint32 nRegister, nTemp;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	RequireEMMS();

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	nRegister	= AllocateRegister();
	nTemp		= AllocateRegister();

	//We need to load the lower value of this variable twice

	//movq nReg + 0, [nVariable2 + 0]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nRegister + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable2 + 0);

	//movq nReg + 1, nReg + 0
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0xC0 | ((nRegister + 1) << 3) | (nRegister + 0));

	//Load the lower value of the other variable into a temp register
	
	//movq nTemp + 0, [nVariable1 + 0]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nTemp + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable1 + 0);

	//punpcklwd nReg + 0, nTemp 
	m_pBlock->StreamWrite(3, 0x0F, 0x61, 0xC0 | ((nRegister + 0) << 3) | (nTemp + 0));

	//punpckhwd nReg + 1, nTemp
	m_pBlock->StreamWrite(3, 0x0F, 0x69, 0xC0 | ((nRegister + 1) << 3) | (nTemp + 0));

	FreeRegister(nTemp);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::UnpackLowerWD()
{
	uint32 nVariable1, nVariable2;
	uint32 nRegister, nTemp;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	RequireEMMS();

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	nRegister	= AllocateRegister();
	nTemp		= AllocateRegister();

	//We need to load the lower value of this variable twice

	//movq nReg + 0, [nVariable2 + 0]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nRegister + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable2 + 0);

	//movq nReg + 1, nReg + 0
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0xC0 | ((nRegister + 1) << 3) | (nRegister + 0));

	//Load the lower value of the other variable into a temp register
	
	//movq nTemp + 0, [nVariable1 + 0]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nTemp + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable1 + 0);

	//punpckldq nReg + 0, nTemp 
	m_pBlock->StreamWrite(3, 0x0F, 0x62, 0xC0 | ((nRegister + 0) << 3) | (nTemp + 0));

	//punpckhqd nReg + 1, nTemp
	m_pBlock->StreamWrite(3, 0x0F, 0x6A, 0xC0 | ((nRegister + 1) << 3) | (nTemp + 0));

	FreeRegister(nTemp);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::UnpackUpperWD()
{
	uint32 nVariable1, nVariable2;
	uint32 nRegister, nTemp;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	RequireEMMS();

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	nRegister	= AllocateRegister();
	nTemp		= AllocateRegister();

	//We need to load the lower value of this variable twice

	//movq nReg + 0, [nVariable2 + 8]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nRegister + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable2 + 8);

	//movq nReg + 1, nReg + 0
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0xC0 | ((nRegister + 1) << 3) | (nRegister + 0));

	//Load the lower value of the other variable into a temp register
	
	//movq nTemp + 0, [nVariable1 + 8]
	m_pBlock->StreamWrite(3, 0x0F, 0x6F, 0x00 | ((nTemp + 0) << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable1 + 8);

	//punpckldq nReg + 0, nTemp 
	m_pBlock->StreamWrite(3, 0x0F, 0x62, 0xC0 | ((nRegister + 0) << 3) | (nTemp + 0));

	//punpckhqd nReg + 1, nTemp
	m_pBlock->StreamWrite(3, 0x0F, 0x6A, 0xC0 | ((nRegister + 1) << 3) | (nTemp + 0));

	FreeRegister(nTemp);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSEFactory::Xor()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pxor mm? + 0, [nVariable + 0]
		m_pBlock->StreamWrite(3, 0x0F, 0xEF, 0x00 | ((nRegister + 0) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 0);

		//pxor mm? + 1, [nVariable + 8]
		m_pBlock->StreamWrite(3, 0x0F, 0xEF, 0x00 | ((nRegister + 1) << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable + 8);
	}
	EndTwoOperands(nRegister);
}

/////////////////////////////////////////////
//SSE2 Factory Implementation
/////////////////////////////////////////////

void CVUI128::CSSE2Factory::Begin(CCacheBlock* pBlock)
{
	unsigned int i;

	m_pBlock = pBlock;
	for(i = 0; i < MAX_REGISTER; i++)
	{
		m_nRegisterAllocated[i] = false;
	}
}

void CVUI128::CSSE2Factory::End()
{
	
}

unsigned int CVUI128::CSSE2Factory::AllocateRegister()
{
	unsigned int i;

	for(i = 0; i < MAX_REGISTER; i++)
	{
		if(m_nRegisterAllocated[i] == false)
		{
			m_nRegisterAllocated[i] = true;
			return i;
		}
	}

	assert(0);
	return 0;
}

void CVUI128::CSSE2Factory::FreeRegister(unsigned int nRegister)
{
	m_nRegisterAllocated[nRegister] = false;
}

void CVUI128::CSSE2Factory::BeginTwoOperands(uint32* nDstRegister, uint32* nSrcVariable)
{
	uint32 nVariable1, nVariable2, nRegister;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	nRegister = AllocateRegister();

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	//movdqa xmm?, [nVariable1]
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x6F, 0x00 | (nRegister << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable1);

	(*nDstRegister) = nRegister;
	(*nSrcVariable) = nVariable2;
}

void CVUI128::CSSE2Factory::EndTwoOperands(uint32 nRegister)
{
	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSE2Factory::LoadVariableInRegister(uint32 nVariable, unsigned int nRegister)
{
	//movdqa xmm?, [nVariable1]
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x6F, 0x00 | (nRegister << 3) | 0x05);
	m_pBlock->StreamWriteWord(nVariable);
}

void CVUI128::CSSE2Factory::Pull(uint128* pValue)
{
	unsigned int nRegister;

	assert(m_OpStack.GetAt(0) == REGISTER);

	m_OpStack.Pull();
	nRegister = m_OpStack.Pull();
	FreeRegister(nRegister);

	//movdqa [pValue], xmm?
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x7F, 0x00 | (nRegister << 3) | 0x05);
	m_pBlock->StreamWriteWord((uint32)((uint8*)(pValue) - (uint8*)0));
}

void CVUI128::CSSE2Factory::AddH()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//paddw xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xFD, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::And()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pand xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xDB, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::CmpEqW()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pcmpeqd xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x76, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::CmpGtH()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pcmpgtw xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x65, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::MaxH()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pmaxsw xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xEE, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::MinH()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pminsw xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xEA, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::Not()
{
	uint32 nRegister1, nRegister2;

	assert(m_OpStack.GetAt(0) == REGISTER);

	nRegister2 = AllocateRegister();

	m_OpStack.Pull();
	nRegister1 = m_OpStack.Pull();

	//pcmpeqd xmm?, xmm?
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x76, 0xC0 | (nRegister2 << 3) | (nRegister2));

	//pxor xmm?, xmm?
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xEF, 0xC0 | (nRegister1 << 3) | (nRegister2));

	FreeRegister(nRegister2);

	m_OpStack.Push(nRegister1);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSE2Factory::Or()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//por xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xEB, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::PackHB()
{
	uint32 nVariable1, nVariable2, nRegister1, nRegister2, nMask;

	assert(m_OpStack.GetAt(0) == VARIABLE);
	assert(m_OpStack.GetAt(2) == VARIABLE);

	m_OpStack.Pull();
	nVariable2 = m_OpStack.Pull();
	m_OpStack.Pull();
	nVariable1 = m_OpStack.Pull();

	nRegister1	= AllocateRegister();
	nRegister2	= AllocateRegister();
	nMask		= AllocateRegister();

	LoadVariableInRegister(nVariable2, nRegister1);
	LoadVariableInRegister(nVariable1, nRegister2);

	//Load 0x00FF x 8 in a temporary register

	//pcmpeqd nMask, nMask
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x76, 0xC0 | (nMask << 3) | (nMask));

	//psrlw nMask, nAmount
	m_pBlock->StreamWrite(5, 0x66, 0x0F, 0x71, 0xD0 | (nMask), 0x08);

	//pand nRegister1, nMask
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xDB, 0xC0 | (nRegister1 << 3) | (nMask));

	//pand nRegister2, nMask
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xDB, 0xC0 | (nRegister2 << 3) | (nMask));

	//packuswb nRegister1, nRegister2
	m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x67, 0xC0 | (nRegister1 << 3) | nRegister2);

	FreeRegister(nRegister2);
	FreeRegister(nMask);

	m_OpStack.Push(nRegister1);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSE2Factory::SubB()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//psubb xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xF8, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::SubW()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//psubd xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xFA, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::SllH(unsigned int nAmount)
{
	uint32 nVariable, nRegister;

	assert(m_OpStack.GetAt(0) == VARIABLE);

	nRegister = AllocateRegister();

	m_OpStack.Pull();
	nVariable = m_OpStack.Pull();

	LoadVariableInRegister(nVariable, nRegister);

	//psllw xmm?, nAmount
	m_pBlock->StreamWrite(5, 0x66, 0x0F, 0x71, 0xF0 | (nRegister), nAmount);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSE2Factory::SrlH(unsigned int nAmount)
{
	uint32 nVariable, nRegister;

	assert(m_OpStack.GetAt(0) == VARIABLE);

	nRegister = AllocateRegister();

	m_OpStack.Pull();
	nVariable = m_OpStack.Pull();

	LoadVariableInRegister(nVariable, nRegister);

	//psrlw xmm?, nAmount
	m_pBlock->StreamWrite(5, 0x66, 0x0F, 0x71, 0xD0 | (nRegister), nAmount);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSE2Factory::SraH(unsigned int nAmount)
{
	uint32 nVariable, nRegister;

	assert(m_OpStack.GetAt(0) == VARIABLE);

	nRegister = AllocateRegister();

	m_OpStack.Pull();
	nVariable = m_OpStack.Pull();

	LoadVariableInRegister(nVariable, nRegister);

	//psraw xmm?, nAmount
	m_pBlock->StreamWrite(5, 0x66, 0x0F, 0x71, 0xE0 | (nRegister), nAmount);

	m_OpStack.Push(nRegister);
	m_OpStack.Push(REGISTER);
}

void CVUI128::CSSE2Factory::UnpackLowerBH()
{
	uint32 nVariable, nRegister;
	
	//Reverse the order of operands on the stack
	nRegister = m_OpStack.GetAt(3);
	m_OpStack.SetAt(3, m_OpStack.GetAt(1));
	m_OpStack.SetAt(1, nRegister);

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//punpcklbw xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x60, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::UnpackUpperBH()
{
	uint32 nVariable, nRegister;
	
	//Reverse the order of operands on the stack
	nRegister = m_OpStack.GetAt(3);
	m_OpStack.SetAt(3, m_OpStack.GetAt(1));
	m_OpStack.SetAt(1, nRegister);

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//punpckhbw xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x68, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::UnpackLowerHW()
{
	uint32 nVariable, nRegister;
	
	//Reverse the order of operands on the stack
	nRegister = m_OpStack.GetAt(3);
	m_OpStack.SetAt(3, m_OpStack.GetAt(1));
	m_OpStack.SetAt(1, nRegister);

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//punpcklwd xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x61, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::UnpackLowerWD()
{
	uint32 nVariable, nRegister;
	
	//Reverse the order of operands on the stack
	nRegister = m_OpStack.GetAt(3);
	m_OpStack.SetAt(3, m_OpStack.GetAt(1));
	m_OpStack.SetAt(1, nRegister);

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//punpckldq xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x62, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::UnpackUpperWD()
{
	uint32 nVariable, nRegister;
	
	//Reverse the order of operands on the stack
	nRegister = m_OpStack.GetAt(3);
	m_OpStack.SetAt(3, m_OpStack.GetAt(1));
	m_OpStack.SetAt(1, nRegister);

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//punpckhdq xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0x6A, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}

void CVUI128::CSSE2Factory::Xor()
{
	uint32 nVariable, nRegister;

	BeginTwoOperands(&nRegister, &nVariable);
	{
		//pxor xmm?, [nVariable]
		m_pBlock->StreamWrite(4, 0x66, 0x0F, 0xEF, 0x00 | (nRegister << 3) | 0x05);
		m_pBlock->StreamWriteWord(nVariable);
	}
	EndTwoOperands(nRegister);
}
