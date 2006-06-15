#include <assert.h>
#include <stdio.h>
#include "IOP_SysMem.h"

using namespace IOP;
using namespace Framework;

void CSysMem::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x01:
		assert(nRetSize == 4);
		*(uint32*)pRet = Allocate(*(uint32*)pArgs);
		break;
	case 0x04:
		assert(nRetSize == 4);
		*(uint32*)pRet = AllocateSystemMemory(((uint32*)pArgs)[0], ((uint32*)pArgs)[1], ((uint32*)pArgs)[2]);
		break;
	default:
		//assert(0);
		break;
	}
}

void CSysMem::SaveState(CStream* pStream)
{

}

void CSysMem::LoadState(CStream* pStream)
{

}

uint32 CSysMem::Allocate(uint32 nSize)
{
	printf("IOP_SysMem: Allocate(size = 0x%0.8X);\r\n", nSize);
	//return 0x01;
	return nSize;
}

uint32 CSysMem::AllocateSystemMemory(uint32 nFlags, uint32 nSize, uint32 nPtr)
{
	//Ys 1&2 Eternal Story calls this

	printf("IOP_SysMem: AllocateSystemMemory(flags = 0x%0.8X, size = 0x%0.8X, ptr = 0x%0.8X);\r\n", nFlags, nSize, nPtr);
	return 0x01;
}
