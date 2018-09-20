#include <stdexcept>
#include "Ps2_PsfDevice.h"
#include "PtrStream.h"
#include "stricmp.h"

using namespace PS2;

void CPsfDevice::AppendArchive(const CPsfBase& archive)
{
	m_fileSystem.AppendArchive(archive);
}

Framework::CStream* CPsfDevice::GetFile(uint32 mode, const char* path)
{
	if(mode != CDevice::OPEN_FLAG_RDONLY)
	{
		printf("%s: Attempting to open a file in non read mode.\r\n", __FUNCTION__);
		return nullptr;
	}
	Framework::CStream* result = m_fileSystem.GetFile(path);
	return result;
}

Iop::Ioman::Directory CPsfDevice::GetDirectory(const char* path)
{
	throw std::runtime_error("Not supported.");
}