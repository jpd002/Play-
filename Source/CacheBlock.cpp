#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stddef.h>
#include "PtrMacro.h"
#include "CacheBlock.h"
#include "MIPS.h"

#ifdef AMD64

#include <windows.h>

extern "C" RET_CODE _CCacheBlock_Execute(void*, void*);

#endif

#define GROWSIZE	(0x40000)

CCacheBlock::CCacheBlock(uint32 nStart, uint32 nEnd)
{
	m_nStart	= nStart;
	m_nEnd		= nEnd;

	m_pData		= (uint8*)malloc(0);
	m_nSize		= 0;
	m_nPtr		= 0;

	m_pReloc	= (uint32*)malloc((nEnd - nStart));

	m_nValid			= false;
	m_nCheckDelayedJump	= false;
}

CCacheBlock::~CCacheBlock()
{
	DELETEPTR(m_pData);
	DELETEPTR(m_pReloc);
}

RET_CODE CCacheBlock::Execute(CMIPS* pCtx)
{
	void* pEntry;
	RET_CODE nRet;
	uint32 nAddress;

	if(!m_nValid)
	{
		Compile(pCtx);

#ifdef AMD64

		DWORD nOldProtect;
		VirtualProtect(m_pData, m_nSize, PAGE_EXECUTE_READWRITE, &nOldProtect);

#endif

	}

	nAddress = pCtx->m_pAddrTranslator(pCtx, 0x00000000, pCtx->m_State.nPC);

	pEntry = (uint8*)m_pData + m_pReloc[(nAddress - m_nStart) / 4];

#ifdef AMD64

	nRet = _CCacheBlock_Execute(pEntry, pCtx);

#else

	__asm
	{
		push	ebx
		push	esi
		push	edi
		push	ebp

		mov		eax, [pEntry];
		mov		ebp, [pCtx];
		call	eax

		pop		ebp
		pop		edi
		pop		esi
		pop		ebx

		mov		[nRet], eax;
	}

#endif

	return nRet;
}

void CCacheBlock::Compile(CMIPS* pCtx)
{
	uint32 i;
	bool nJump;

	m_nCheckDelayedJump = pCtx->IsBranch(m_nStart - 4);

	//Check if there's a jump instruction before this one
	for(i = m_nStart; i < m_nEnd; i += 4)
	{
		m_pReloc[(i - m_nStart) / 4] = m_nPtr;

		m_nProgramCounterChanged = false;
		nJump = m_nCheckDelayedJump;

		InsertProlog(pCtx);
		pCtx->m_pArch->CompileInstruction(i, this, pCtx, true);
		InsertEpilog(pCtx, nJump);

	}
	//Check jump out of the block
	Ret(RET_CODE_INTERBLOCKJUMP);
	m_nValid = true;
	printf("CacheBlock: Compiled block [0x%0.8X, 0x%0.8X]. %i byte(s) allocated.\r\n", m_nStart, m_nEnd, m_nSize);
}

void CCacheBlock::InsertProlog(CMIPS* pCtx)
{
	//add [pCtx->m_State.nPC], 4
	StreamWrite(4, 0x83, (0x01 << 6) | (0x00 << 3) | (0x05), offsetof(CMIPS, m_State.nPC), 4);
}

void CCacheBlock::InsertEpilog(CMIPS* pCtx, bool nDelayJump)
{
	//Check delay slot (we truly need to do something better about this...)
	if(nDelayJump)
	{
		m_nCheckDelayedJump = false;

		//mov eax, dword ptr[nDelayedJumpAddr]
		StreamWrite(3, 0x8B, (0x01 << 6) | (0x00 << 3) | (0x05), offsetof(CMIPS, m_State.nDelayedJumpAddr));

		//cmp eax, 1
		StreamWrite(1, 0x3D);
		StreamWriteWord(MIPS_INVALID_PC);

		//je +0B
		StreamWrite(2, 0x74, 0x0B);

		//mov dword ptr[nPC], eax
		StreamWrite(3, 0x89, (0x01 << 6) | (0x00 << 3) | (0x05), offsetof(CMIPS, m_State.nPC));

		//mov eax, 1
		StreamWrite(1, 0xB8);
		StreamWriteWord(MIPS_INVALID_PC);

		//mov dword ptr[nDelayedJumpAddr], eax
		StreamWrite(3, 0x89, (0x01 << 6) | (0x00 << 3) | (0x05), offsetof(CMIPS, m_State.nDelayedJumpAddr));

		//PC Changed
		SetProgramCounterChanged();
	}

	if(pCtx->m_pTickFunction != NULL)
	{
		//Call the tick function (push a different value than 1 if there's a dead loop in there)

#ifdef AMD64
		
		//mov ecx, 1
		StreamWrite(1, 0xB9);
		StreamWriteWord(1);

		//sub esp, 0x08
		StreamWrite(3, 0x83, 0xC0 | (0x05 << 3) | (0x04), 0x08);

#else

		//push 1
		StreamWrite(2, 0x6A, 0x01);

#endif

		//call [TickFunction]
		StreamWrite(2, 0xFF, (0x02 << 6) | (0x02 << 3) | (0x05));
		StreamWriteWord(offsetof(CMIPS, m_pTickFunction));

#ifdef AMD64

		//add esp, 0x08
		StreamWrite(3, 0x83, 0xC0 | (0x00 << 3) | (0x04), 0x08);

#else

		//add esp, 4
		StreamWrite(3, 0x83, 0xC4, 0x04);

#endif

		//test eax, eax
		StreamWrite(2, 0x85, 0xC0);

		//je +0x06
		StreamWrite(2, 0x74, 0x06);

		//mov eax, RET_CODE_BREAKPOINT
		StreamWrite(1, 0xB8);
		StreamWriteWord(RET_CODE_BREAKPOINT);

		//ret
		StreamWrite(1, 0xC3);
	}

	//////////////////////////////
	//Decrement quota

	//sub [Quota], 1
	StreamWrite(2, 0x83, (0x02 << 6) | (0x05 << 3) | (0x05));
	StreamWriteWord(offsetof(CMIPS, m_nQuota));
	StreamWriteByte(1);

	//jne $notdone
	StreamWriteByte(0x75);
	StreamWriteByte(0x06);

	//mov eax, nCode
	StreamWriteByte(0xB8);
	StreamWriteWord(RET_CODE_QUOTADONE);

	//ret (stack unwinding?)
	StreamWriteByte(0xC3);

	////////////////////////////////////////////////////

	//Take the jump
	if(m_nProgramCounterChanged)
	{
		SynchornizePC(pCtx);
	}

}

void CCacheBlock::SynchornizePC(CMIPS* pCtx)
{
	//mov eax, dword ptr[nPC]
	StreamWrite(3, 0x8B, (0x01 << 6) | (0x00 << 3) | (0x05), offsetof(CMIPS, m_State.nPC));

	//sub eax, m_nStart
	StreamWrite(1, 0x2D);
	StreamWriteWord(m_nStart);

	//jns +0x06
	StreamWrite(2, 0x79, 0x06);

	//mov eax, RET_CODE_INTERBLOCKJUMP
	StreamWrite(1, 0xB8);
	StreamWriteWord(RET_CODE_INTERBLOCKJUMP);

	//ret
	StreamWrite(1, 0xC3);

	//cmp eax, m_nSize
	StreamWrite(1, 0x3D);
	StreamWriteWord(m_nEnd - m_nStart);

	//jb +0x06
	StreamWrite(2, 0x72, 0x06);

	//mov eax, RET_CODE_INTERBLOCKJUMP
	StreamWrite(1, 0xB8);
	StreamWriteWord(RET_CODE_INTERBLOCKJUMP);

	//ret
	StreamWrite(1, 0xC3);

	//mov ebx, m_nReloc (64-BITS : Potentially dangerous!)
	StreamWrite(1, 0xBB);
	StreamWriteWord((uint32)((uint8*)(m_pReloc) - (uint8*)0));

	//mov eax, [ebx + eax]
	StreamWrite(3, 0x8B, 0x00 | (0x00 << 3) | (0x04), 0x03);

	//add eax, m_pData (64-BITS : Potentially dangerous!)
	StreamWrite(1, 0x05);
	StreamWriteWord((uint32)((uint8*)(m_pData) - (uint8*)0));

	//jmp eax
	StreamWrite(2, 0xFF, 0xE0);
}

///////////////////////////////////////////////////////////
//Stream Fonction Implementation
///////////////////////////////////////////////////////////

void CCacheBlock::StreamWriteByte(uint8 nByte)
{
	if(m_nPtr == m_nSize)
	{
		m_nSize += GROWSIZE;
		m_pData = (uint8*)realloc(m_pData, m_nSize);
	}
	m_pData[m_nPtr] = nByte;
	m_nPtr++;
}

void CCacheBlock::StreamWriteWord(uint32 nWord)
{
	unsigned int i;
	for(i = 0; i < 4; i++)
	{
		StreamWriteByte(((uint8*)&nWord)[i]);
	}
}

void CCacheBlock::StreamWrite(unsigned int nCount, ...)
{
	va_list args;
	uint8 nValue;
	unsigned int i;

	va_start(args, nCount);
	
	for(i = 0; i < nCount; i++)
	{
		nValue = va_arg(args, int);
		StreamWriteByte(static_cast<uint8>(nValue));
	}

	va_end(args);
}

unsigned int CCacheBlock::StreamGetSize()
{
	return m_nPtr;
}

void CCacheBlock::StreamWriteAt(unsigned int nPosition, uint8 nValue)
{
	m_pData[nPosition] = nValue;
}

///////////////////////////////////////////////////////////
//Intermediate Language Implementation
///////////////////////////////////////////////////////////

void CCacheBlock::PushAddr(void* pAddr)
{
	//push [pAddr]
	StreamWriteByte(0xFF);
	StreamWriteByte(0x35);
	StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
}

void CCacheBlock::PullAddr(void* pAddr)
{
	//pop [pAddr]
	StreamWriteByte(0x8F);
	StreamWriteByte(0x05);
	StreamWriteWord((uint32)((uint8*)pAddr - (uint8*)0));
}

void CCacheBlock::PushImm(uint32 nValue)
{
	//push nValue
	StreamWriteByte(0x68);
	StreamWriteWord(nValue);
}

void CCacheBlock::PushRef(void* pRef)
{
	PushImm((uint32)((uint8*)pRef - (uint8*)0));
}

void CCacheBlock::SeX8()
{
	//pop eax
	StreamWriteByte(0x58);

	//movsx eax, al
	StreamWriteByte(0x0F);
	StreamWriteByte(0xBE);
	StreamWriteByte(0xC0);

	//push eax
	StreamWriteByte(0x50);
}

void CCacheBlock::SeX16()
{
	//pop eax
	StreamWriteByte(0x58);

	//cwde
	StreamWriteByte(0x98);

	//push eax
	StreamWriteByte(0x50);
}

void CCacheBlock::SeX32()
{
	//pop eax
	StreamWriteByte(0x58);

	//cdq
	StreamWriteByte(0x99);

	//push eax
	StreamWriteByte(0x50);

	//push edx
	StreamWriteByte(0x52);
}

void CCacheBlock::PullTop()
{
	Free(1);
}

void CCacheBlock::Free(unsigned int nAmount)
{
	nAmount *= 4;

	assert(nAmount < 128);

	//add esp, 4
	StreamWriteByte(0x83);
	StreamWriteByte(0xC4);
	StreamWriteByte((uint8)nAmount);
}

void CCacheBlock::PushTop()
{
	//push [esp]
	StreamWriteByte(0xFF);
	StreamWriteByte(0x34);
	StreamWriteByte(0x24);
}

void CCacheBlock::PushValueAt(unsigned int nAddress)
{
	nAddress *= 4;
	
	assert(nAddress < 128);

	//push [esp + nAddress];
	StreamWriteByte(0xFF);
	StreamWriteByte(0x74);
	StreamWriteByte(0x24);
	StreamWriteByte((uint8)nAddress);
}

void CCacheBlock::Lookup(uint32* pTable)
{
	//pop eax
	StreamWriteByte(0x58);

	//mov ebx, $pTable
	StreamWriteByte(0xBB);
	StreamWriteWord((uint32)((uint8*)pTable - (uint8*)0));

	//mov eax, [eax * 4 + ebx]
	StreamWriteByte(0x8B);
	StreamWriteByte(0x04);
	StreamWriteByte(0x83);

	//push eax
	StreamWriteByte(0x50);
}

void CCacheBlock::Exchange(unsigned int nA1, unsigned int nA2)
{
	nA1 *= 4;
	nA2 *= 4;

	assert(nA1 < 128);
	assert(nA2 < 128);

	//mov eax, [esp + nA1]
	StreamWriteByte(0x8B);
	StreamWriteByte(0x44);
	StreamWriteByte(0x24);
	StreamWriteByte((uint8)nA1);

	//xchg eax, [esp + nA2]
	StreamWriteByte(0x87);
	StreamWriteByte(0x44);
	StreamWriteByte(0x24);
	StreamWriteByte((uint8)nA2);

	//mov [esp + nA1], eax
	StreamWriteByte(0x89);
	StreamWriteByte(0x44);
	StreamWriteByte(0x24);
	StreamWriteByte((uint8)nA1);
}

void CCacheBlock::Swap()
{
	Exchange(0, 1);
}

void CCacheBlock::And()
{
	//pop eax
	StreamWriteByte(0x58);

	//and [esp], eax
	StreamWriteByte(0x21);
	StreamWriteByte(0x04);
	StreamWriteByte(0x24);
}

void CCacheBlock::AndImm(uint32 nValue)
{
	//and [esp], nValue
	StreamWriteByte(0x81);
	StreamWriteByte(0x24);
	StreamWriteByte(0x24);
	StreamWriteWord(nValue);
}

void CCacheBlock::Add()
{
	//pop eax
	StreamWriteByte(0x58);

	//add [esp], eax
	StreamWriteByte(0x01);
	StreamWriteByte(0x04);
	StreamWriteByte(0x24);
}

void CCacheBlock::AddC()
{
	//pop eax
	StreamWriteByte(0x58);

	//pop ebx
	StreamWriteByte(0x5B);

	//push 0
	StreamWriteByte(0x6A);
	StreamWriteByte(0x00);

	//add ebx, eax
	StreamWriteByte(0x03);
	StreamWriteByte(0xD8);

	//adc [esp], 0
	StreamWriteByte(0x80);
	StreamWriteByte(0x14);
	StreamWriteByte(0x24);
	StreamWriteByte(0x00);

	//push ebx
	StreamWriteByte(0x53);
}

void CCacheBlock::AddImm(uint32 nValue)
{
	if(nValue == 0) return;

	//add [esp], nValue
	StreamWriteByte(0x81);
	StreamWriteByte(0x04);
	StreamWriteByte(0x24);
	StreamWriteWord(nValue);
}

void CCacheBlock::Sub()
{
	//pop eax
	StreamWriteByte(0x58);

	//sub [esp], eax
	StreamWriteByte(0x29);
	StreamWriteByte(0x04);
	StreamWriteByte(0x24);
}

void CCacheBlock::SubImm(uint32 nValue)
{
	//sub [esp], nValue
	StreamWriteByte(0x81);
	StreamWriteByte(0x2C);
	StreamWriteByte(0x24);
	StreamWriteWord(nValue);
}

void CCacheBlock::SubC()
{
	//pop eax
	StreamWriteByte(0x58);

	//pop ebx
	StreamWriteByte(0x5B);

	//push 0
	StreamWriteByte(0x6A);
	StreamWriteByte(0x00);

	//sub ebx, eax
	StreamWriteByte(0x2B);
	StreamWriteByte(0xD8);

	//adc [esp], 0
	StreamWriteByte(0x80);
	StreamWriteByte(0x14);
	StreamWriteByte(0x24);
	StreamWriteByte(0x00);

	//push ebx
	StreamWriteByte(0x53);
}

void CCacheBlock::PushConditionBit(JCC_CONDITION nCond)
{
	switch(nCond)
	{
	case JCC_CONDITION_EQ:
		//je
		StreamWriteByte(0x74);
		break;
	case JCC_CONDITION_NE:
		//jne
		StreamWriteByte(0x75);
		break;
	case JCC_CONDITION_BL:
		//jb
		StreamWriteByte(0x72);
		break;
	case JCC_CONDITION_BE:
		//jbe
		StreamWriteByte(0x76);
		break;
	case JCC_CONDITION_LT:
		//jl
		StreamWriteByte(0x7C);
		break;
	case JCC_CONDITION_LE:
		//jle
		StreamWriteByte(0x7E);
		break;
	default:
		assert(0);
		break;
	}

	//$04 (label for previous jump)
	StreamWriteByte(0x04);

	//push 0
	StreamWriteByte(0x6A);
	StreamWriteByte(0x00);

	//jmp $02
	StreamWriteByte(0xEB);
	StreamWriteByte(0x02);

	//push 1
	StreamWriteByte(0x6A);
	StreamWriteByte(0x01);
}

void CCacheBlock::Cmp(JCC_CONDITION nCond)
{
	//pop eax
	StreamWriteByte(0x58);

	//cmp [esp], eax
	StreamWriteByte(0x39);
	StreamWriteByte(0x04);
	StreamWriteByte(0x24);

	//pop eax
	StreamWriteByte(0x58);

	PushConditionBit(nCond);
}

void CCacheBlock::Call(void* pFunc, unsigned int nParams, bool nSaveRes)
{
/*
	//call [pFunc]
	StreamWrite(2, 0xFF, 0x00 | (0x02 << 3) | (0x05));
	StreamWriteWord((uint32)((uint8*)(pFunc) - (uint8*)0));
*/
	//mov eax, pFunc
	StreamWriteByte(0xB8);
	StreamWriteWord((uint32)((uint8*)pFunc - (uint8*)0));

	//call eax
	StreamWriteByte(0xFF);
	StreamWriteByte(0xD0);

	if(nParams != 0)
	{
		//add esp, [nParams * 4];
		StreamWriteByte(0x83);
		StreamWriteByte(0xC4);
		StreamWriteByte(nParams * 4);
	}

	if(nSaveRes)
	{
		//push eax
		StreamWriteByte(0x50);
	}
}

void CCacheBlock::BeginJcc(bool nCond)
{
	//pop eax
	StreamWriteByte(0x58);

	//test eax, eax
	StreamWriteByte(0x85);
	StreamWriteByte(0xC0);

	if(!nCond)
	{
		//jne $l
		StreamWriteByte(0x75);
	}
	else
	{
		//je $l
		StreamWriteByte(0x74);
	}

	m_nJccPos = m_nPtr;
	StreamWriteByte(0x00);
}

void CCacheBlock::EndJcc()
{
	uint8 nDist;
	nDist = (uint8)(m_nPtr - (m_nJccPos + 1));
	m_pData[m_nJccPos] = nDist;
}

void CCacheBlock::Ret(RET_CODE nCode)
{
	//mov eax, nCode
	StreamWriteByte(0xB8);
	StreamWriteWord(nCode);

	//ret (stack unwinding?)
	StreamWriteByte(0xC3);
}

void CCacheBlock::Mult()
{
	//pop eax
	StreamWriteByte(0x58);

	//pop ebx
	StreamWriteByte(0x5B);

	//mul ebx
	StreamWriteByte(0xF7);
	StreamWriteByte(0xE3);

	//push edx
	StreamWriteByte(0x52);

	//push eax
	StreamWriteByte(0x50);
}

void CCacheBlock::MultS()
{
	//pop eax
	StreamWriteByte(0x58);

	//pop ebx
	StreamWriteByte(0x5B);

	//imul ebx
	StreamWriteByte(0xF7);
	StreamWriteByte(0xEB);

	//push edx
	StreamWriteByte(0x52);

	//push eax
	StreamWriteByte(0x50);
}

void CCacheBlock::Div()
{
	//pop ebx
	StreamWriteByte(0x5B);

	//pop eax
	StreamWriteByte(0x58);

	//xor edx, edx
	StreamWriteByte(0x33);
	StreamWriteByte(0xD2);

	//div ebx
	StreamWriteByte(0xF7);
	StreamWriteByte(0xF3);

	//push edx
	StreamWriteByte(0x52);

	//push eax
	StreamWriteByte(0x50);
}

void CCacheBlock::DivS()
{
	//pop ebx
	StreamWriteByte(0x5B);

	//pop eax
	StreamWriteByte(0x58);

	//cdq
	StreamWriteByte(0x99);

	//idiv ebx
	StreamWriteByte(0xF7);
	StreamWriteByte(0xFB);

	//push edx
	StreamWriteByte(0x52);

	//push eax
	StreamWriteByte(0x50);
}

void CCacheBlock::Or()
{
	//pop eax
	StreamWriteByte(0x58);

	//or [esp], eax
	StreamWriteByte(0x09);
	StreamWriteByte(0x04);
	StreamWriteByte(0x24);
}

void CCacheBlock::OrImm(uint32 nValue)
{
	//or [esp], nValue
	StreamWriteByte(0x81);
	StreamWriteByte(0x0C);
	StreamWriteByte(0x24);
	StreamWriteWord(nValue);
}

void CCacheBlock::Xor()
{
	//pop eax
	StreamWriteByte(0x58);

	//xor [esp], eax
	StreamWriteByte(0x31);
	StreamWriteByte(0x04);
	StreamWriteByte(0x24);
}

void CCacheBlock::Not()
{
	//not [esp]
	StreamWriteByte(0xF7);
	StreamWriteByte(0x14);
	StreamWriteByte(0x24);
}

void CCacheBlock::Sll()
{
	//pop ecx
	StreamWriteByte(0x59);

	//shl [esp], cl
	StreamWriteByte(0xD3);
	StreamWriteByte(0x24);
	StreamWriteByte(0x24);
}

void CCacheBlock::SllImm(uint8 nAmount)
{
	//shl [esp], nAmount
	StreamWriteByte(0xC1);
	StreamWriteByte(0x24);
	StreamWriteByte(0x24);
	StreamWriteByte(nAmount);
}

void CCacheBlock::Sll64()
{
	//pop ecx
	StreamWriteByte(0x59);

	//pop edx
	StreamWriteByte(0x5A);

	//pop eax
	StreamWriteByte(0x58);

	//test cl, cl
	StreamWriteByte(0x84);
	StreamWriteByte(0xC9);

	//je $done
	StreamWriteByte(0x74);
	StreamWriteByte(0x20);

	//cmp cl, 0x40
	StreamWriteByte(0x80);
	StreamWriteByte(0xF9);
	StreamWriteByte(0x40);

	//jae $zero
	StreamWriteByte(0x73);
	StreamWriteByte(0x17);

	//cmp cl, 0x20
	StreamWriteByte(0x80);
	StreamWriteByte(0xF9);
	StreamWriteByte(0x20);

	//jae $more32
	StreamWriteByte(0x73);
	StreamWriteByte(0x07);

	//shld edx, eax, cl
	StreamWriteByte(0x0F);
	StreamWriteByte(0xA5);
	StreamWriteByte(0xC2);

	//shl eax, cl
	StreamWriteByte(0xD3);
	StreamWriteByte(0xE0);

	//jmp $done
	StreamWriteByte(0xEB);
	StreamWriteByte(0x0F);

	//$more32

	//mov edx, eax
	StreamWriteByte(0x8B);
	StreamWriteByte(0xD0);
	
	//xor eax, eax
	StreamWriteByte(0x33);
	StreamWriteByte(0xC0);

	//and cl, 0x1F
	StreamWriteByte(0x80);
	StreamWriteByte(0xE1);
	StreamWriteByte(0x1F);

	//shl edx, cl
	StreamWriteByte(0xD3);
	StreamWriteByte(0xE2);

	//jmp $done
	StreamWriteByte(0xEB);
	StreamWriteByte(0x04);

	//$zero

	//xor eax, eax
	StreamWriteByte(0x33);
	StreamWriteByte(0xC0);

	//xor edx, edx
	StreamWriteByte(0x33);
	StreamWriteByte(0xD2);

	//$done

	//push eax
	StreamWriteByte(0x50);

	//push edx
	StreamWriteByte(0x52);
}

void CCacheBlock::Srl()
{
	//pop ecx
	StreamWriteByte(0x59);

	//shr [esp], cl
	StreamWriteByte(0xD3);
	StreamWriteByte(0x2C);
	StreamWriteByte(0x24);
}

void CCacheBlock::SrlImm(uint8 nAmount)
{
	//shr [esp], nAmount
	StreamWriteByte(0xC1);
	StreamWriteByte(0x2C);
	StreamWriteByte(0x24);
	StreamWriteByte(nAmount);
}

void CCacheBlock::Srl64()
{
	//pop ecx
	StreamWriteByte(0x59);

	//pop edx
	StreamWriteByte(0x5A);

	//pop eax
	StreamWriteByte(0x58);

	//test cl, cl
	StreamWriteByte(0x84);
	StreamWriteByte(0xC9);

	//je $done
	StreamWriteByte(0x74);
	StreamWriteByte(0x20);

	//cmp cl, 0x40
	StreamWriteByte(0x80);
	StreamWriteByte(0xF9);
	StreamWriteByte(0x40);

	//jae $zero
	StreamWriteByte(0x73);
	StreamWriteByte(0x17);

	//cmp cl, 0x20
	StreamWriteByte(0x80);
	StreamWriteByte(0xF9);
	StreamWriteByte(0x20);

	//jae $more32
	StreamWriteByte(0x73);
	StreamWriteByte(0x07);

	//shrd eax, edx, cl
	StreamWriteByte(0x0F);
	StreamWriteByte(0xAD);
	StreamWriteByte(0xD0);

	//shr edx, cl
	StreamWriteByte(0xD3);
	StreamWriteByte(0xEA);

	//jmp $done
	StreamWriteByte(0xEB);
	StreamWriteByte(0x0F);

	//$more32

	//mov eax, edx
	StreamWriteByte(0x8B);
	StreamWriteByte(0xC2);
	
	//xor edx, edx
	StreamWriteByte(0x33);
	StreamWriteByte(0xD2);

	//and cl, 0x1F
	StreamWriteByte(0x80);
	StreamWriteByte(0xE1);
	StreamWriteByte(0x1F);

	//shr eax, cl
	StreamWriteByte(0xD3);
	StreamWriteByte(0xE8);

	//jmp $done
	StreamWriteByte(0xEB);
	StreamWriteByte(0x04);

	//$zero

	//xor eax, eax
	StreamWriteByte(0x33);
	StreamWriteByte(0xC0);

	//xor edx, edx
	StreamWriteByte(0x33);
	StreamWriteByte(0xD2);

	//$done

	//push eax
	StreamWriteByte(0x50);

	//push edx
	StreamWriteByte(0x52);

}

void CCacheBlock::Sra()
{
	//pop ecx
	StreamWriteByte(0x59);

	//sar [esp], cl
	StreamWriteByte(0xD3);
	StreamWriteByte(0x3C);
	StreamWriteByte(0x24);
}

void CCacheBlock::SraImm(uint8 nAmount)
{
	//sar [esp], nAmount
	StreamWriteByte(0xC1);
	StreamWriteByte(0x3C);
	StreamWriteByte(0x24);
	StreamWriteByte(nAmount);
}

void CCacheBlock::Sra64()
{
	//pop ecx
	StreamWriteByte(0x59);

	//pop edx
	StreamWriteByte(0x5A);

	//pop eax
	StreamWriteByte(0x58);

	//cmp cl, 0x40
	StreamWrite(3, 0x80, 0xF9, 0x40);

	//jae $more64
	StreamWrite(2, 0x73, 0x18);

	//cmp cl, 0x40
	StreamWrite(3, 0x80, 0xF9, 0x20);

	//jae $more32
	StreamWrite(2, 0x73, 0x07);

	//shrd eax, edx, cl
	StreamWrite(3, 0x0F, 0xAD, 0xD0);

	//sar edx, cl
	StreamWrite(2, 0xD3, 0xFA);

	//jmp $done
	StreamWrite(2, 0xEB, 0x11);

	//more32:

	//mov eax, edx
	StreamWrite(2, 0x8B, 0xC2);

	//sar edx, 0x1F
	StreamWrite(3, 0xC1, 0xFA, 0x1F);

	//and cl, 0x1F
	StreamWrite(3, 0x80, 0xE1, 0x1F);

	//sar eax, cl
	StreamWrite(2, 0xD3, 0xF8);

	//jmp $done
	StreamWrite(2, 0xEB, 0x05);

	//more64:

	//sar edx, 0x1F
	StreamWrite(3, 0xC1, 0xFA, 0x1F);

	//mov eax, edx
	StreamWrite(2, 0x8B, 0xC2);
/*
	__asm
	{
        cmp     cl,64
		jae		more64

		cmp     cl, 32
        jae     more32

		shrd    eax,edx,cl
        sar     edx,cl

		jmp		done

more32:
        mov     eax,edx
        sar     edx,31
        and     cl,31
        sar     eax,cl

		jmp		done

more64:
        sar     edx,31
        mov     eax,edx

done:
	}
*/
	//push eax
	StreamWriteByte(0x50);

	//push edx
	StreamWriteByte(0x52);
}

void CCacheBlock::SetDelayedJumpCheck()
{
	m_nCheckDelayedJump = true;
}

void CCacheBlock::SetProgramCounterChanged()
{
	m_nProgramCounterChanged = true;
}

///////////////////////////////////////////////////////////
//Proxies Implementations
///////////////////////////////////////////////////////////

uint32 CCacheBlock::GetByteProxy(CMIPS* pCtx, uint32 nAddress)
{
	return (uint32)pCtx->m_pMemoryMap->GetByte(nAddress);
}

uint32 CCacheBlock::GetHalfProxy(CMIPS* pCtx, uint32 nAddress)
{
	return (uint32)pCtx->m_pMemoryMap->GetHalf(nAddress);
}

uint32 CCacheBlock::GetWordProxy(CMIPS* pCtx, uint32 nAddress)
{
	return pCtx->m_pMemoryMap->GetWord(nAddress);
}

void CCacheBlock::SetByteProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetByte(nAddress, (uint8)(nValue & 0xFF));
}

void CCacheBlock::SetHalfProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetHalf(nAddress, (uint16)(nValue & 0xFFFF));
}

void CCacheBlock::SetWordProxy(CMIPS* pCtx, uint32 nValue, uint32 nAddress)
{
	pCtx->m_pMemoryMap->SetWord(nAddress, nValue);
}
