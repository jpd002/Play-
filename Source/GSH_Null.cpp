#include "GSH_Null.h"

using namespace std::tr1;

CGSH_Null::CGSH_Null()
{

}

CGSH_Null::~CGSH_Null()
{

}

void CGSH_Null::InitializeImpl()
{

}

void CGSH_Null::ReleaseImpl()
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

CGSHandler::FactoryFunction CGSH_Null::GetFactoryFunction()
{
    return bind(&CGSH_Null::GSHandlerFactory);
}

CGSHandler* CGSH_Null::GSHandlerFactory()
{
	return new CGSH_Null();
}
