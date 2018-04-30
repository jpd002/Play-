#include "Psp_PsfDevice.h"
#include "Psp_IoFileMgrForUser.h"

using namespace Psp;

CPsfDevice::CPsfDevice()
{
}

CPsfDevice::~CPsfDevice()
{
}

Framework::CStream* CPsfDevice::GetFile(const char* path, uint32 flags)
{
	if(flags != CIoFileMgrForUser::OPEN_READ)
	{
		return NULL;
	}
	Framework::CStream* result = m_fileSystem.GetFile(path);
	return result;
}

void CPsfDevice::AppendArchive(const CPsfBase& archive)
{
	m_fileSystem.AppendArchive(archive);
}
