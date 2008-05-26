#include <assert.h>
#include "Iop_Unknown2.h"

//This module is also used in Castlevania : Yami no Juin

using namespace Iop;
using namespace std;

CUnknown2::CUnknown2(CSIF& sif)
{
    sif.RegisterModule(MODULE_ID, this);
}

CUnknown2::~CUnknown2()
{

}

string CUnknown2::GetId() const
{
    return "unknown2";
}

void CUnknown2::Invoke(CMIPS&, unsigned int)
{
    throw runtime_error("Not implemented.");
}

void CUnknown2::Invoke(uint32 nMethod, uint32* pArgs, uint32 nArgsSize, uint32* pRet, uint32 nRetSize, uint8* ram)
{
	switch(nMethod)
	{
	case 1:
	case 2:
		assert(nRetSize >= 4);

		((uint32*)pRet)[0] = 1;

		break;
	}
}

void CUnknown2::SaveState(CZipArchiveWriter& archive)
{	

}

void CUnknown2::LoadState(CZipArchiveReader& archive)
{

}
