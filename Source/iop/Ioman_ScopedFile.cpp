#include "iop/Ioman_ScopedFile.h"

using namespace Iop;
using namespace Iop::Ioman;

CScopedFile::CScopedFile(uint32 handle, CIoman& ioman)
    : m_handle(handle)
    , m_ioman(ioman)
{
}

CScopedFile::~CScopedFile()
{
	m_ioman.Close(m_handle);
}

CScopedFile::operator uint32()
{
	return m_handle;
}
