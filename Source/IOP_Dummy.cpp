#include <assert.h>
#include "IOP_Dummy.h"

using namespace IOP;
using namespace Framework;

void CDummy::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
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

void CDummy::LoadState(CStream* pStream)
{

}

void CDummy::SaveState(CStream* pStream)
{

}
