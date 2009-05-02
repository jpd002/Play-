#include <assert.h>
#include "CodeGen.h"
#include "CodeGen_StackPatterns.h"
#include "placeholder_def.h"

using namespace std;

void CCodeGen::MD_PushRel(size_t offset)
{
    m_Shadow.Push(static_cast<uint32>(offset));
    m_Shadow.Push(RELATIVE128);
}

void CCodeGen::MD_PushRelExpand(size_t offset)
{
	throw std::exception();
}

void CCodeGen::MD_PushCstExpand(float constant)
{
	throw std::exception();
}

void CCodeGen::MD_PullRel(size_t offset)
{
	throw std::exception();
}

void CCodeGen::MD_PullRel(size_t offset0, size_t offset1, size_t offset2, size_t offset3)
{
	throw std::exception();
}

void CCodeGen::MD_PullRel(size_t registerId, bool has0, bool has1, bool has2, bool has3)
{
	throw std::exception();
}

void CCodeGen::MD_AbsS()
{
	throw std::exception();
}

void CCodeGen::MD_AddH()
{
	throw std::exception();
}

void CCodeGen::MD_AddWSS()
{
	throw std::exception();
}

void CCodeGen::MD_AddWUS()
{
	throw std::exception();
}

void CCodeGen::MD_AddS()
{
	throw std::exception();
}

void CCodeGen::MD_And()
{
	throw std::exception();
}

void CCodeGen::MD_CmpEqW()
{
	throw std::exception();
}

void CCodeGen::MD_CmpGtH()
{
	throw std::exception();
}

void CCodeGen::MD_DivS()
{
	throw std::exception();
}

void CCodeGen::MD_IsNegative()
{
	throw std::exception();
}

void CCodeGen::MD_IsZero()
{
	throw std::exception();
}

void CCodeGen::MD_MaxH()
{
	throw std::exception();
}

void CCodeGen::MD_MaxS()
{
	throw std::exception();
}

void CCodeGen::MD_MinH()
{
	throw std::exception();
}

void CCodeGen::MD_MinS()
{
	throw std::exception();
}

void CCodeGen::MD_MulS()
{
	throw std::exception();
}

void CCodeGen::MD_Not()
{
	throw std::exception();
}

void CCodeGen::MD_Or()
{
	throw std::exception();
}

void CCodeGen::MD_PackHB()
{
	throw std::exception();
}

void CCodeGen::MD_PackWH()
{
	throw std::exception();
}

void CCodeGen::MD_SllH(uint8 amount)
{
	throw std::exception();
}

void CCodeGen::MD_SraH(uint8 amount)
{
	throw std::exception();
}

void CCodeGen::MD_SraW(uint8 amount)
{
	throw std::exception();
}

void CCodeGen::MD_SrlH(uint8 amount)
{
	throw std::exception();
}

void CCodeGen::MD_Srl256()
{
	throw std::exception();
}

void CCodeGen::MD_SubB()
{
	throw std::exception();
}

void CCodeGen::MD_SubW()
{
	throw std::exception();
}

void CCodeGen::MD_SubS()
{
	throw std::exception();
}

void CCodeGen::MD_ToSingle()
{
	throw std::exception();
}

void CCodeGen::MD_ToWordTruncate()
{
	throw std::exception();
}

void CCodeGen::MD_UnpackLowerBH()
{
	throw std::exception();
}

void CCodeGen::MD_UnpackLowerHW()
{
	throw std::exception();
}

void CCodeGen::MD_UnpackLowerWD()
{
	throw std::exception();
}

void CCodeGen::MD_UnpackUpperBH()
{
	throw std::exception();
}

void CCodeGen::MD_UnpackUpperWD()
{
	throw std::exception();
}

void CCodeGen::MD_Xor()
{
	throw std::exception();
}
