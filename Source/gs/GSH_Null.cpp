#include "GSH_Null.h"

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

void CGSH_Null::ProcessImageTransfer()
{

}

void CGSH_Null::ProcessClutTransfer(uint32, uint32)
{

}

void CGSH_Null::ProcessLocalToLocalTransfer()
{

}

void CGSH_Null::ReadFramebuffer(uint32, uint32, void*)
{

}

CGSHandler::FactoryFunction CGSH_Null::GetFactoryFunction()
{
	return std::bind(&CGSH_Null::GSHandlerFactory);
}

CGSHandler* CGSH_Null::GSHandlerFactory()
{
	return new CGSH_Null();
}
