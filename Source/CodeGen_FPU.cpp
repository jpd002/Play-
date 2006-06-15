#include <assert.h>
#include "CodeGen_FPU.h"
#include "PtrMacro.h"

using namespace CodeGen;

CCacheBlock*	CFPU::m_pBlock = NULL;

void CFPU::Begin(CCacheBlock* pBlock)
{
	m_pBlock = pBlock;
}

void CFPU::End()
{

}

void CFPU::PushSingle(void* pAddr)
{
	//fld dword ptr[pAddr]
	m_pBlock->StreamWrite(2, 0xD9, 0x05);
	m_pBlock->StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
}

void CFPU::PushSingleImm(float nValue)
{
	if(nValue == 0.0)
	{
		//fldz
		m_pBlock->StreamWrite(2, 0xD9, 0xEE);
	}
	else
	{
		//push nValue
		m_pBlock->StreamWrite(1, 0x68);
		m_pBlock->StreamWriteWord(*(uint32*)&nValue);

		//fld dword ptr [esp]
		m_pBlock->StreamWrite(3, 0xD9, 0x04, 0x24);

		//add esp, 4
		m_pBlock->StreamWrite(3, 0x83, 0xC4, 0x04);
	}
}

void CFPU::PushWord(void* pAddr)
{
	//fild dword ptr[pAddr]
	m_pBlock->StreamWrite(2, 0xDB, 0x00 | (0x00 << 3) | 0x05);
	m_pBlock->StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
}

void CFPU::PullSingle(void* pAddr)
{
	//fstp dword ptr[pAddr]
	m_pBlock->StreamWrite(2, 0xD9, 0x00 | (0x03 << 3) | 0x05);
	m_pBlock->StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
}

void CFPU::PullWord(void* pAddr)
{
	//fistp dword ptr[pAddr]
	m_pBlock->StreamWrite(2, 0xDB, 0x00 | (0x03 << 3) | 0x05);
	m_pBlock->StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
}

void CFPU::PushRoundingMode()
{
	//sub esp, 4
	m_pBlock->StreamWrite(3, 0x83, 0xEC, 0x04);

	//fwait
	m_pBlock->StreamWrite(1, 0x9B);

	//fnstcw word ptr [esp]
	m_pBlock->StreamWrite(3, 0xD9, 0x3C, 0x24);
}

void CFPU::PullRoundingMode()
{
	//fldcw word ptr [esp]
	m_pBlock->StreamWrite(3, 0xD9, 0x2C, 0x24);

	//add esp, 4
	m_pBlock->StreamWrite(3, 0x83, 0xC4, 0x04);
}

void CFPU::SetRoundingMode(ROUNDMODE nMode)
{
	//sub esp, 4
	m_pBlock->StreamWrite(3, 0x83, 0xEC, 0x04);

	//fwait
	m_pBlock->StreamWrite(1, 0x9B);

	//fnstcw word ptr [esp]
	m_pBlock->StreamWrite(3, 0xD9, 0x3C, 0x24);

	//pop eax
	m_pBlock->StreamWrite(1, 0x58);

	//and eax, 0xFFFFF3FF
	m_pBlock->StreamWrite(1, 0x25);
	m_pBlock->StreamWriteWord(0xFFFFF3FF);

	//or eax, nMode << 10
	m_pBlock->StreamWrite(1, 0x0D);
	m_pBlock->StreamWriteWord(nMode << 10);

	//push eax
	m_pBlock->StreamWrite(1, 0x50);

	//fldcw word ptr [esp]
	m_pBlock->StreamWrite(3, 0xD9, 0x2C, 0x24);

	//add esp, 4
	m_pBlock->StreamWrite(3, 0x83, 0xC4, 0x04);
}

void CFPU::Add()
{
	//faddp
	m_pBlock->StreamWrite(2, 0xDE, 0xC1);
}

void CFPU::Abs()
{
	//fabs
	m_pBlock->StreamWrite(2, 0xD9, 0xE1);
}

void CFPU::Dup()
{
	//fld st
	m_pBlock->StreamWrite(2, 0xD9, 0xC0);
}

void CFPU::Sub()
{
	//fsubp
	m_pBlock->StreamWrite(2, 0xDE, 0xE9);
}

void CFPU::Mul()
{
	//fmulp
	m_pBlock->StreamWrite(2, 0xDE, 0xC9);
}

void CFPU::Div()
{
	//fdivp
	m_pBlock->StreamWrite(2, 0xDE, 0xF9);
}

void CFPU::Neg()
{
	//fchs
	m_pBlock->StreamWrite(2, 0xD9, 0xE0);
}

void CFPU::Rcpl()
{
	//fld1
	m_pBlock->StreamWrite(2, 0xD9, 0xE8);

	//fdivrp
	m_pBlock->StreamWrite(2, 0xDE, 0xF1);
}

void CFPU::Sqrt()
{
	//fsqrt
	m_pBlock->StreamWrite(2, 0xD9, 0xFA);
}

void CFPU::Cmp(CCodeGen::CONDITION nCondition)
{
	unsigned int nRegister;

	//fcomip
	m_pBlock->StreamWrite(2, 0xDF, 0xF1);

	//fstp
	m_pBlock->StreamWrite(2, 0xDD, 0xD8);

	nRegister = CCodeGen::AllocateRegister(CCodeGen::REGISTER_HASLOW);

	switch(nCondition)
	{
	case CCodeGen::CONDITION_EQ:
		//sete reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x94, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister]));
		break;
	case CCodeGen::CONDITION_BL:
		//setb reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x92, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister]));
		break;
	case CCodeGen::CONDITION_BE:
		//setbe reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x96, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister]));
		break;
	case CCodeGen::CONDITION_AB:
		//seta reg[l]
		m_pBlock->StreamWrite(3, 0x0F, 0x97, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister]));
		break;
	default:
		assert(0);
		break;
	}

	//movzx reg, reg[l]
	m_pBlock->StreamWrite(3, 0x0F, 0xB6, 0xC0 | (CCodeGen::m_nRegisterLookup[nRegister] << 3) | (CCodeGen::m_nRegisterLookup[nRegister]));

	CCodeGen::PushReg(nRegister);
}

void CFPU::Round()
{
	//frndint
	m_pBlock->StreamWrite(2, 0xD9, 0xFC);
}
