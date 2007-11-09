#include "Ioman_Psf.h"
#include "PtrStream.h"

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
    if(flags != CDevice::O_RDONLY)
    {
        printf("%s: Attempting to open a file in non read mode.\r\n", __FUNCTION__);
        return NULL;
    }
    const CPsfFs::FILE* file = m_fileSystem.GetFile(path);
    if(file == NULL)
    {
        return NULL;
    }
    return new CPtrStream(file->data, file->size);
}
