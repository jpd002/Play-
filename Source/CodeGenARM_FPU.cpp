#include <assert.h>
#include "CodeGen.h"
#include "CodeGen_StackPatterns.h"
#include "placeholder_def.h"

using namespace std;

void CCodeGen::FP_PushSingle(size_t offset)
{
    m_Shadow.Push(static_cast<uint32>(offset));
    m_Shadow.Push(FP_SINGLE_RELATIVE);
}

void CCodeGen::FP_PushWord(size_t offset)
{
	throw std::exception();
}

void CCodeGen::FP_PullSingle(size_t offset)
{
	throw std::exception();
}

void CCodeGen::FP_PushCst(float constant)
{
	throw std::exception();
}

void CCodeGen::FP_PullWordTruncate(size_t offset)
{
	throw std::exception();
}

void CCodeGen::FP_Add()
{
	throw std::exception();
}

void CCodeGen::FP_Abs()
{
	throw std::exception();
}

void CCodeGen::FP_Sub()
{
	throw std::exception();
}

void CCodeGen::FP_Mul()
{
	throw std::exception();
}

void CCodeGen::FP_Div()
{
	throw std::exception();
}

void CCodeGen::FP_Cmp(CCodeGen::CONDITION condition)
{
	throw std::exception();
}

void CCodeGen::FP_Neg()
{
	throw std::exception();
}

void CCodeGen::FP_Rcpl()
{
	throw std::exception();
}

void CCodeGen::FP_Sqrt()
{
	throw std::exception();
}

void CCodeGen::FP_Rsqrt()
{
	throw std::exception();
}
