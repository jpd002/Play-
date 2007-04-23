#include "GSH_Null.h"
#include "PS2VM.h"

CGSH_Null::CGSH_Null()
{

}

CGSH_Null::~CGSH_Null()
{

}

void CGSH_Null::UpdateViewport()
{

}

void CGSH_Null::ProcessImageTransfer(uint32 nAddress, uint32 nSize)
{

}

void CGSH_Null::Flip()
{
    CPS2VM::m_OnNewFrame();
}

void CGSH_Null::CreateGSHandler()
{
	CPS2VM::CreateGSHandler(GSHandlerFactory, NULL);
}

CGSHandler* CGSH_Null::GSHandlerFactory(void* pParam)
{
	return new CGSH_Null();
}
