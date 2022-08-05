#include "GSH_Null.h"

void CGSH_Null::InitializeImpl()
{
}

void CGSH_Null::ReleaseImpl()
{
}

void CGSH_Null::ProcessHostToLocalTransfer()
{
}

void CGSH_Null::ProcessLocalToHostTransfer()
{
}

void CGSH_Null::ProcessLocalToLocalTransfer()
{
}

void CGSH_Null::ProcessClutTransfer(uint32, uint32)
{
}

CGSHandler::FactoryFunction CGSH_Null::GetFactoryFunction()
{
	return []() { return new CGSH_Null(); };
}
