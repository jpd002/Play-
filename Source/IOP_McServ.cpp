#include <assert.h>
#include "IOP_McServ.h"

using namespace IOP;
using namespace Framework;

CMcServ::CMcServ()
{
	
}

CMcServ::~CMcServ()
{

}

void CMcServ::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0xFE:
		//Get version?
		GetVersionInformation(pArgs, nArgsSize, pRet, nRetSize);
		break;
	default:
		//assert(0);
		break;
	}
}

void CMcServ::SaveState(CStream* pStream)
{

}

void CMcServ::LoadState(CStream* pStream)
{

}

void CMcServ::GetVersionInformation(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nArgsSize == 0x30);
	assert(nRetSize == 0x0C);

	((uint32*)pRet)[0] = 0x00000000;
	((uint32*)pRet)[1] = 0x0000020A;		//mcserv version
	((uint32*)pRet)[2] = 0x0000020E;		//mcman version
}
