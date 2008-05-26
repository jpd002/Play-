#include "IOP_Unknown.h"

//This module is required for Castlevania: Yami no Juin to work.

using namespace Iop;
using namespace std;

CUnknown::CUnknown(CSIF& sif)
{
    sif.RegisterModule(MODULE_ID, this);
}

CUnknown::~CUnknown()
{

}

string CUnknown::GetId() const
{
    return "unknown";
}

void CUnknown::Invoke(CMIPS&, unsigned int)
{
    throw runtime_error("Not implemented.");
}

void CUnknown::Invoke(uint32 nMethod, uint32* pArgs, uint32 nArgsSize, uint32* pRet, uint32 nRetSize, uint8* ram)
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

void CUnknown::SaveState(CZipArchiveWriter& archive)
{	

}

void CUnknown::LoadState(CZipArchiveReader& archive)
{

}
