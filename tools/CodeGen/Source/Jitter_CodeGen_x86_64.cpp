#include "Jitter_CodeGen_x86_64.h"

using namespace Jitter;

CCodeGen_x86_64::CCodeGen_x86_64()
{

}

CCodeGen_x86_64::~CCodeGen_x86_64()
{

}

unsigned int CCodeGen_x86_64::GetAvailableRegisterCount() const
{
	return 7;
}
