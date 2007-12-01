#include "GSH_Null.h"
#include "PS2VM.h"

CGSH_Null::CGSH_Null()
{

}

CGSH_Null::~CGSH_Null()
{

}

void CGSH_Null::InitializeImpl()
{

}

void CGSH_Null::UpdateViewportImpl()
{

}

void CGSH_Null::ProcessImageTransfer(uint32 nAddress, uint32 nSize)
{

}

void CGSH_Null::FlipImpl()
{
//    CPS2VM::m_OnNewFrame();
}

void CGSH_Null::CreateGSHandler(CPS2VM& virtualMachine)
{
	virtualMachine.CreateGSHandler(GSHandlerFactory, NULL);
}

CGSHandler* CGSH_Null::GSHandlerFactory(void* pParam)
{
	return new CGSH_Null();
}
