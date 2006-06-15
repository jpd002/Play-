#include "IOP_LibSD.h"

using namespace IOP;
using namespace Framework;

void CLibSD::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x8100:
		//Not sure about this one
		GetBufferSize(pArgs, nArgsSize, pRet, nRetSize);
		break;
	}
}

void CLibSD::SaveState(CStream* pStream)
{

}

void CLibSD::LoadState(CStream* pStream)
{

}

void CLibSD::GetBufferSize(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	static uint32 nTemp = 0;
	nTemp += 0x400;
	((uint32*)pRet)[0] = nTemp;
}
