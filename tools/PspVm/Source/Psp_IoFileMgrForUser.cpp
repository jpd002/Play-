#include "Psp_IoFileMgrForUser.h"
#include "Log.h"
#include "alloca_def.h"

#define LOGNAME	("Psp_IoFileMgrForUser")

using namespace Psp;

CIoFileMgrForUser::CIoFileMgrForUser(uint8* ram)
: m_ram(ram)
{
	m_nextFileId = FD_FIRSTUSERID;
}

CIoFileMgrForUser::~CIoFileMgrForUser()
{

}

std::string CIoFileMgrForUser::GetName() const
{
	return "IoFileMgrForUser";
}

void CIoFileMgrForUser::RegisterDevice(const char* name, const IoDevicePtr& device)
{
	m_devices[name] = device;
}

uint32 CIoFileMgrForUser::IoOpen(uint32 pathAddr, uint32 flags, uint32 mode)
{
	assert(pathAddr != 0);
	char* path = NULL;
	if(pathAddr != 0)
	{
		path = reinterpret_cast<char*>(m_ram + pathAddr);
	}
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "IoOpen(path = %s, flags = 0x%0.8X, mode = 0x%0.8X);\r\n",
		path, flags, mode);
#endif

	std::string fullPath(path);
	std::string::size_type position = fullPath.find(":");
	if(position == std::string::npos) 
    {
#ifdef _DEBUG
		CLog::GetInstance().Print(LOGNAME, "Couldn't open file. Invalid path.\r\n");
#endif
		return -1;
    }
	std::string deviceName(fullPath.begin(), fullPath.begin() + position);
	std::string devicePath(fullPath.begin() + position + 1, fullPath.end());
    IoDeviceList::iterator deviceIterator(m_devices.find(deviceName));
	if(deviceIterator == m_devices.end())
	{
#ifdef _DEBUG
		CLog::GetInstance().Print(LOGNAME, "Couldn't open file. Device not found.\r\n");
#endif
		return -1;
	}
	Framework::CStream* file = deviceIterator->second->GetFile(devicePath.c_str(), flags);
	if(file == NULL)
	{
#ifdef _DEBUG
		CLog::GetInstance().Print(LOGNAME, "Couldn't open file. Device returned error.\r\n");
#endif
		return -1;
	}

	int fileId = m_nextFileId++;
	m_files[fileId] = StreamPtr(file);
	return fileId;
}

uint32 CIoFileMgrForUser::IoRead(uint32 fd, uint32 bufferAddr, uint32 bufferSize)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "IoRead(%d, 0x%0.8X, 0x%X);\r\n",
		fd, bufferAddr, bufferSize);
#endif
	assert(bufferAddr != 0);
	uint8* buffer = m_ram + bufferAddr;

	FileList::iterator fileIterator = m_files.find(fd);
	if(fileIterator == m_files.end())
	{
#ifdef _DEBUG
		CLog::GetInstance().Print(LOGNAME, "Invalid file descriptor.\r\n");
#endif
		return -1;
	}
	uint32 result = 0;
	result = static_cast<uint32>(fileIterator->second->Read(buffer, bufferSize));
	return result;
}

uint32 CIoFileMgrForUser::IoWrite(uint32 fd, uint32 bufferAddr, uint32 bufferSize)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "IoWrite(%d, 0x%0.8X, 0x%X);\r\n",
		fd, bufferAddr, bufferSize);
#endif
	assert(bufferAddr != 0);
	uint8* buffer = m_ram + bufferAddr;
	if(fd == FD_STDOUT)
	{
		char* data = reinterpret_cast<char*>(_alloca(bufferSize + 1));
		memcpy(data, buffer, bufferSize);
		data[bufferSize] = 0;
		printf("%s", data);		
	}
	return bufferSize;
}

uint32 CIoFileMgrForUser::IoLseek(uint32 fd, uint32 offset, uint32 whence)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "IoLseek(%d, 0x%0.8X, %d);\r\n",
		fd, offset, whence);
#endif

	FileList::iterator fileIterator = m_files.find(fd);
	if(fileIterator == m_files.end())
	{
#ifdef _DEBUG
		CLog::GetInstance().Print(LOGNAME, "Invalid file descriptor.\r\n");
#endif
		return -1;
	}
	uint32 result = 0;
	Framework::STREAM_SEEK_DIRECTION direction;
	switch(whence)
	{
	case 0:
		direction = Framework::STREAM_SEEK_SET;
		break;
	case 1:
		direction = Framework::STREAM_SEEK_CUR;
		break;
	case 2:
		direction = Framework::STREAM_SEEK_END;
		break;
	}
	fileIterator->second->Seek(offset, direction);
	result = static_cast<uint32>(fileIterator->second->Tell());
	return result;
}

uint32 CIoFileMgrForUser::IoClose(uint32 fd)
{
#ifdef _DEBUG
	CLog::GetInstance().Print(LOGNAME, "IoClose(%d);\r\n", fd);
#endif

	FileList::iterator fileIterator = m_files.find(fd);
	if(fileIterator == m_files.end())
	{
#ifdef _DEBUG
		CLog::GetInstance().Print(LOGNAME, "Invalid file descriptor.\r\n");
#endif
		return -1;
	}

	m_files.erase(fd);
	return 0;
}

void CIoFileMgrForUser::Invoke(uint32 methodId, CMIPS& context)
{
	switch(methodId)
	{
	case 0x109F50BC:
		context.m_State.nGPR[CMIPS::V0].nV0 = IoOpen(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 0x6A638D83:
		context.m_State.nGPR[CMIPS::V0].nV0 = IoRead(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 0x42EC03AC:
		context.m_State.nGPR[CMIPS::V0].nV0 = IoWrite(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 0x27EB27B8:
		context.m_State.nGPR[CMIPS::V0].nV0 = IoLseek(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0,
			context.m_State.nGPR[CMIPS::T0].nV0);
		break;
	case 0x810C4BC3:
		context.m_State.nGPR[CMIPS::V0].nV0 = IoClose(
			context.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X\r\n", methodId);
		break;
	}
}
