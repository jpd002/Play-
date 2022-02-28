#include "Iop_IomanX.h"
#include "Iop_Ioman.h"

using namespace Iop;

//IOMANX is an extended version of IOMAN used by some homebrew ELF files
//On a real console, that module will hook IOMAN's functions and supplement its functionality
//This HLE module will just redirect any calls to the original IOMAN module

CIomanX::CIomanX(CIoman& ioman)
    : m_ioman(ioman)
{
}

std::string CIomanX::GetId() const
{
	return "iomanx";
}

std::string CIomanX::GetFunctionName(unsigned int functionId) const
{
	return m_ioman.GetFunctionName(functionId);
}

void CIomanX::Invoke(CMIPS& context, unsigned int functionId)
{
	m_ioman.Invoke(context, functionId);
}

void CIomanX::SaveState(Framework::CZipArchiveWriter& archive) const
{
}

void CIomanX::LoadState(Framework::CZipArchiveReader& archive)
{
}
