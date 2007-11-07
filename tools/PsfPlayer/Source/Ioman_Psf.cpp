#include "Ioman_Psf.h"

using namespace Iop::Ioman;
using namespace Framework;

CPsf::CPsf(CPsfFs& fileSystem) :
m_fileSystem(fileSystem)
{
    
}

CPsf::~CPsf()
{

}

CStream* CPsf::GetFile(uint32 flags, const char* path)
{
    return NULL;
}
