#include "PS2VM.h"
#include "IOP_Unknown.h"

using namespace IOP;

void CUnknown::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x01000000:
		{
			if(nRetSize == 0x7C)
			{
				(reinterpret_cast<uint16*>(pRet))[0x30] |= 0x7;
				(reinterpret_cast<uint16*>(pRet))[0x31] |= 0x7;

				(reinterpret_cast<uint16*>(pRet))[0x30] ^= 0x8;
				(reinterpret_cast<uint16*>(pRet))[0x31] ^= 0x8;
			}
			break;
		}
	}
}

void CUnknown::LoadState(Framework::CStream* pStream)
{

}

void CUnknown::SaveState(Framework::CStream* pStream)
{

}
