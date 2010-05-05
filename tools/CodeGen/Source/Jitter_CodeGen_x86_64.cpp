#include "Jitter_CodeGen_x86_64.h"

using namespace Jitter;

CX86Assembler::REGISTER CCodeGen_x86_64::g_registers[7] =
{
	CX86Assembler::rBX,
	CX86Assembler::rSI,
	CX86Assembler::rDI,
};

CCodeGen_x86_64::CONSTMATCHER CCodeGen_x86_64::g_constMatchers[] = 
{ 
	{ OP_PARAM,		MATCH_NIL,			MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_x86_64::Emit_Param_Cst64		},
};

CCodeGen_x86_64::CCodeGen_x86_64()
{
	CCodeGen_x86::m_registers = g_registers;
}

CCodeGen_x86_64::~CCodeGen_x86_64()
{

}

unsigned int CCodeGen_x86_64::GetAvailableRegisterCount() const
{
	return 7;
}
