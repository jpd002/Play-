#include <cstring>
#include <stdexcept>
#include <cctype>

#include "StdStream.h"
#include "xml/Utils.h"
#include "std_experimental_map.h"

#include "Iop_Ioman.h"
#include "IopBios.h"
#include "../AppConfig.h"
#include "../Log.h"
#include "../states/XmlStateFile.h"
#include "ioman/PathDirectoryDevice.h"

using namespace Iop;

#define LOG_NAME "iop_ioman"

#define STATE_FILES_FILENAME ("iop_ioman/files.xml")
#define STATE_FILES_FILESNODE "Files"
#define STATE_FILES_FILENODE "File"
#define STATE_FILES_FILENODE_IDATTRIBUTE ("Id")
#define STATE_FILES_FILENODE_PATHATTRIBUTE ("Path")
#define STATE_FILES_FILENODE_FLAGSATTRIBUTE ("Flags")
#define STATE_FILES_FILENODE_DESCPTRATTRIBUTE ("DescPtr")

#define STATE_USERDEVICES_FILENAME ("iop_ioman/userdevices.xml")
#define STATE_USERDEVICES_DEVICESNODE "Devices"
#define STATE_USERDEVICES_DEVICENODE "Device"
#define STATE_USERDEVICES_DEVICENODE_NAMEATTRIBUTE ("Name")
#define STATE_USERDEVICES_DEVICENODE_DESCPTRATTRIBUTE ("DescPtr")

#define PREF_IOP_FILEIO_STDLOGGING ("iop.fileio.stdlogging")

#define FUNCTION_WRITE "Write"
#define FUNCTION_ADDDRV "AddDrv"
#define FUNCTION_DELDRV "DelDrv"
#define FUNCTION_MOUNT "Mount"
#define FUNCTION_UMOUNT "Umount"
#define FUNCTION_SEEK64 "Seek64"

//Directories have "group read" only permissions? This is required by PS2PSXe.
#define STAT_MODE_DIR (0747 | (1 << 12))  //File mode + Dir type (1)
#define STAT_MODE_FILE (0777 | (2 << 12)) //File mode + File type (2)

/** No such file or directory */
#define ERROR_ENOENT 2

static std::string RightTrim(std::string inputString)
{
	auto nonSpaceEnd = std::find_if(inputString.rbegin(), inputString.rend(), [](int ch) { return !std::isspace(ch); });
	inputString.erase(nonSpaceEnd.base(), inputString.end());
	return inputString;
}

struct PATHINFO
{
	std::string deviceName;
	std::string devicePath;
};

static PATHINFO SplitPath(const char* path)
{
	std::string fullPath(path);
	auto position = fullPath.find(":");
	if(position == std::string::npos)
	{
		throw std::runtime_error("Invalid path.");
	}
	PATHINFO result;
	result.deviceName = std::string(fullPath.begin(), fullPath.begin() + position);
	result.devicePath = std::string(fullPath.begin() + position + 1, fullPath.end());
	//Some games (Street Fighter EX3) provide paths with trailing spaces
	result.devicePath = RightTrim(result.devicePath);
	return result;
}

CIoman::CIoman(CIopBios& bios, uint8* ram)
    : m_bios(bios)
    , m_ram(ram)
    , m_nextFileHandle(3)
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_IOP_FILEIO_STDLOGGING, false);

	//Insert standard files if requested.
	if(CAppConfig::GetInstance().GetPreferenceBoolean(PREF_IOP_FILEIO_STDLOGGING)
#ifdef DEBUGGER_INCLUDED
	   || true
#endif
	)
	{
		try
		{
			auto stdoutPath = CAppConfig::GetBasePath() / "ps2_stdout.txt";
			auto stderrPath = CAppConfig::GetBasePath() / "ps2_stderr.txt";

			m_files[FID_STDOUT] = FileInfo{new Framework::CStdStream(fopen(stdoutPath.string().c_str(), "ab"))};
			m_files[FID_STDERR] = FileInfo{new Framework::CStdStream(fopen(stderrPath.string().c_str(), "ab"))};
		}
		catch(...)
		{
			//Humm, some error occured when opening these files...
		}
	}
}

CIoman::~CIoman()
{
	m_files.clear();
	m_devices.clear();
}

void CIoman::PrepareOpenThunk()
{
	if(m_openThunkPtr != 0) return;

	static const uint32 thunkSize = 0x30;
	auto sysmem = m_bios.GetSysmem();
	m_openThunkPtr = sysmem->AllocateMemory(thunkSize, 0, 0);

	static const int16 stackAlloc = 0x10;

	CMIPSAssembler assembler(reinterpret_cast<uint32*>(m_ram + m_openThunkPtr));

	auto finishLabel = assembler.CreateLabel();

	//Save return value (stored in T0)
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
	assembler.SW(CMIPS::RA, 0x00, CMIPS::SP);

	//Call open handler
	assembler.JALR(CMIPS::A3);
	assembler.SW(CMIPS::T0, 0x04, CMIPS::SP);

	//Check if open handler reported error, if so, leave return value untouched
	//TODO: Release handle if we failed to open properly
	assembler.BLTZ(CMIPS::V0, finishLabel);
	assembler.LW(CMIPS::RA, 0x00, CMIPS::SP);

	assembler.LW(CMIPS::V0, 0x04, CMIPS::SP);

	assembler.MarkLabel(finishLabel);
	assembler.JR(CMIPS::RA);
	assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);

	assert((assembler.GetProgramSize() * 4) <= thunkSize);
}

std::string CIoman::GetId() const
{
	return "ioman";
}

std::string CIoman::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return "open";
		break;
	case 5:
		return "close";
		break;
	case 6:
		return "read";
		break;
	case 7:
		return FUNCTION_WRITE;
		break;
	case 8:
		return "seek";
		break;
	case 16:
		return "getstat";
		break;
	case 20:
		return FUNCTION_ADDDRV;
		break;
	case 21:
		return FUNCTION_DELDRV;
		break;
	default:
		return "unknown";
		break;
	}
}

void CIoman::RegisterDevice(const char* name, const DevicePtr& device)
{
	m_devices[name] = device;
}

uint32 CIoman::Open(uint32 flags, const char* path)
{
	CLog::GetInstance().Print(LOG_NAME, "Open(flags = 0x%08X, path = '%s');\r\n", flags, path);

	int32 handle = PreOpen(flags, path);
	if(handle < 0)
	{
		return handle;
	}

	assert(!IsUserDeviceFileHandle(handle));
	return handle;
}

Framework::CStream* CIoman::OpenInternal(uint32 flags, const char* path)
{
	auto pathInfo = SplitPath(path);
	auto deviceIterator = m_devices.find(pathInfo.deviceName);
	if(deviceIterator == m_devices.end())
	{
		throw std::runtime_error("Device not found.");
	}
	auto stream = deviceIterator->second->GetFile(flags, pathInfo.devicePath.c_str());
	if(!stream)
	{
		throw std::runtime_error("File not found.");
	}
	return stream;
}

uint32 CIoman::Close(uint32 handle)
{
	CLog::GetInstance().Print(LOG_NAME, "Close(handle = %d);\r\n", handle);

	uint32 result = 0xFFFFFFFF;
	assert(!IsUserDeviceFileHandle(handle));
	try
	{
		auto file(m_files.find(handle));
		if(file == std::end(m_files))
		{
			throw std::runtime_error("Invalid file handle.");
		}
		FreeFileHandle(handle);
		//Returns handle instead of 0 (needed by Naruto: Ultimate Ninja 2)
		result = handle;
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occured while trying to close file : %s\r\n", __FUNCTION__, except.what());
	}
	return result;
}

uint32 CIoman::Read(uint32 handle, uint32 size, void* buffer)
{
	CLog::GetInstance().Print(LOG_NAME, "Read(handle = %d, size = 0x%X, buffer = ptr);\r\n", handle, size);

	uint32 result = 0xFFFFFFFF;
	assert(!IsUserDeviceFileHandle(handle));
	try
	{
		auto stream = GetFileStream(handle);
		if(stream->IsEOF())
		{
			result = 0;
		}
		else
		{
			result = static_cast<uint32>(stream->Read(buffer, size));
		}
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occured while trying to read file : %s\r\n", __FUNCTION__, except.what());
	}
	return result;
}

uint32 CIoman::Write(uint32 handle, uint32 size, const void* buffer)
{
	CLog::GetInstance().Print(LOG_NAME, "Write(handle = %d, size = 0x%X, buffer = ptr);\r\n", handle, size);

	uint32 result = 0xFFFFFFFF;
	assert(!IsUserDeviceFileHandle(handle));
	try
	{
		auto stream = GetFileStream(handle);
		if(!stream)
		{
			throw std::runtime_error("Failed to obtain file stream.");
		}
		result = static_cast<uint32>(stream->Write(buffer, size));
		if((handle == FID_STDOUT) || (handle == FID_STDERR))
		{
			//Force flusing stdout and stderr
			stream->Flush();
		}
	}
	catch(const std::exception& except)
	{
		if((handle != FID_STDOUT) && (handle != FID_STDERR))
		{
			CLog::GetInstance().Warn(LOG_NAME, "%s: Error occured while trying to write file : %s\r\n", __FUNCTION__, except.what());
		}
	}
	return result;
}

uint32 CIoman::Seek(uint32 handle, int32 position, uint32 whence)
{
	CLog::GetInstance().Print(LOG_NAME, "Seek(handle = %d, position = %d, whence = %d);\r\n",
	                          handle, position, whence);

	uint32 result = -1U;
	assert(!IsUserDeviceFileHandle(handle));
	try
	{
		auto stream = GetFileStream(handle);
		auto direction = ConvertWhence(whence);
		stream->Seek(position, direction);
		result = static_cast<uint32>(stream->Tell());
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occured while trying to seek file : %s\r\n", __FUNCTION__, except.what());
	}
	return result;
}

int32 CIoman::Mkdir(const char* path)
{
	CLog::GetInstance().Print(LOG_NAME, "Mkdir(path = '%s');\r\n", path);
	try
	{
		auto pathInfo = SplitPath(path);
		auto deviceIterator = m_devices.find(pathInfo.deviceName);
		if(deviceIterator == m_devices.end())
		{
			throw std::runtime_error("Device not found.");
		}

		deviceIterator->second->MakeDirectory(pathInfo.devicePath.c_str());
		return 0;
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occured while trying to create directory : %s\r\n", __FUNCTION__, except.what());
		return -1;
	}
}

int32 CIoman::Dopen(const char* path)
{
	CLog::GetInstance().Print(LOG_NAME, "Dopen(path = '%s');\r\n",
	                          path);
	int32 handle = -1;
	try
	{
		auto pathInfo = SplitPath(path);
		auto deviceIterator = m_devices.find(pathInfo.deviceName);
		if(deviceIterator == m_devices.end())
		{
			throw std::runtime_error("Device not found.");
		}
		auto directory = deviceIterator->second->GetDirectory(pathInfo.devicePath.c_str());
		handle = m_nextFileHandle++;
		m_directories[handle] = directory;
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occured while trying to open directory : %s\r\n", __FUNCTION__, except.what());
	}
	return handle;
}

int32 CIoman::Dclose(uint32 handle)
{
	CLog::GetInstance().Print(LOG_NAME, "Dclose(handle = %d);\r\n",
	                          handle);

	auto directoryIterator = m_directories.find(handle);
	if(directoryIterator == std::end(m_directories))
	{
		return -1;
	}

	m_directories.erase(directoryIterator);
	return 0;
}

int32 CIoman::Dread(uint32 handle, Ioman::DIRENTRY* dirEntry)
{
	CLog::GetInstance().Print(LOG_NAME, "Dread(handle = %d, entry = ptr);\r\n",
	                          handle);

	auto directoryIterator = m_directories.find(handle);
	if(directoryIterator == std::end(m_directories))
	{
		return -1;
	}

	auto& directory = directoryIterator->second;
	if(directory == Ioman::Directory())
	{
		return 0;
	}

	auto itemPath = directory->path();
	auto name = itemPath.filename().string();
	strncpy(dirEntry->name, name.c_str(), Ioman::DIRENTRY::NAME_SIZE);
	dirEntry->name[Ioman::DIRENTRY::NAME_SIZE - 1] = 0;

	auto& stat = dirEntry->stat;
	memset(&stat, 0, sizeof(Ioman::STAT));
	if(fs::is_directory(itemPath))
	{
		stat.mode = STAT_MODE_DIR;
		stat.attr = 0x8427;
	}
	else
	{
		stat.mode = STAT_MODE_FILE;
		stat.loSize = fs::file_size(itemPath);
		stat.attr = 0x8497;
	}

	directory++;

	return strlen(dirEntry->name);
}

uint32 CIoman::GetStat(const char* path, Ioman::STAT* stat)
{
	CLog::GetInstance().Print(LOG_NAME, "GetStat(path = '%s', stat = ptr);\r\n", path);

	//Try with a directory
	{
		int32 fd = Dopen(path);
		if(fd >= 0)
		{
			Dclose(fd);
			memset(stat, 0, sizeof(Ioman::STAT));
			stat->mode = STAT_MODE_DIR;
			return 0;
		}
	}

	//Try with a file
	{
		int32 fd = Open(Ioman::CDevice::OPEN_FLAG_RDONLY, path);
		if(fd >= 0)
		{
			uint32 size = Seek(fd, 0, SEEK_DIR_END);
			Close(fd);
			memset(stat, 0, sizeof(Ioman::STAT));
			stat->mode = STAT_MODE_FILE;
			stat->loSize = size;
			return 0;
		}
	}

	return -1;
}

int32 CIoman::AddDrv(CMIPS& context)
{
	auto devicePtr = context.m_State.nGPR[CMIPS::A0].nV0;

	CLog::GetInstance().Print(LOG_NAME, FUNCTION_ADDDRV "(devicePtr = 0x%08X);\r\n",
	                          devicePtr);

	auto device = reinterpret_cast<const Ioman::DEVICE*>(m_ram + devicePtr);
	auto deviceName = device->namePtr ? reinterpret_cast<const char*>(m_ram + device->namePtr) : nullptr;
	auto deviceDesc = device->descPtr ? reinterpret_cast<const char*>(m_ram + device->descPtr) : nullptr;
	CLog::GetInstance().Print(LOG_NAME, "Requested registration of device '%s'.\r\n", deviceName);
	//We only support "cdfs" for now
	if(!deviceName || strcmp(deviceName, "cdfs"))
	{
		return -1;
	}
	m_userDevices.insert(std::make_pair(deviceName, devicePtr));
	InvokeUserDeviceMethod(context, devicePtr, offsetof(Ioman::DEVICEOPS, initPtr), devicePtr);
	return 0;
}

uint32 CIoman::DelDrv(uint32 drvNamePtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_DELDRV "(drvNamePtr = %s);\r\n",
	                          PrintStringParameter(m_ram, drvNamePtr).c_str());
	return -1;
}

int32 CIoman::Mount(const char* fsName, const char* devicePath)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_MOUNT "(fsName = '%s', devicePath = '%s');\r\n",
	                          fsName, devicePath);

	auto pathInfo = SplitPath(devicePath);
	auto deviceIterator = m_devices.find(pathInfo.deviceName);
	if(deviceIterator == m_devices.end())
	{
		return -1;
	}

	auto device = deviceIterator->second;
	uint32 result = 0;
	try
	{
		auto mountedDeviceName = std::string(fsName);
		//Strip any colons we might have in the string
		mountedDeviceName.erase(std::remove(mountedDeviceName.begin(), mountedDeviceName.end(), ':'), mountedDeviceName.end());
		assert(m_devices.find(mountedDeviceName) == std::end(m_devices));

		auto partitionPath = device->GetMountPath(pathInfo.devicePath.c_str());
		auto mountedDevice = std::make_shared<Ioman::CPathDirectoryDevice>(partitionPath);
		m_devices[mountedDeviceName] = mountedDevice;
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occurred while trying to mount : %s : %s\r\n", __FUNCTION__, devicePath, except.what());
		result = -1;
	}
	return result;
}

int32 CIoman::Umount(const char* deviceName)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_UMOUNT "(deviceName = '%s');\r\n", deviceName);

	auto mountedDeviceName = std::string(deviceName);
	//Strip any colons we might have in the string
	mountedDeviceName.erase(std::remove(mountedDeviceName.begin(), mountedDeviceName.end(), ':'), mountedDeviceName.end());

	auto deviceIterator = m_devices.find(mountedDeviceName);
	if(deviceIterator == std::end(m_devices))
	{
		//Device not found
		return -1;
	}

	//We maybe need to make sure we don't have outstanding fds?
	m_devices.erase(deviceIterator);

	return 0;
}

uint64 CIoman::Seek64(uint32 handle, int64 position, uint32 whence)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_SEEK64 "(handle = %d, position = %ld, whence = %d);\r\n",
	                          handle, position, whence);

	uint64 result = -1ULL;
	assert(!IsUserDeviceFileHandle(handle));
	try
	{
		auto stream = GetFileStream(handle);
		auto direction = ConvertWhence(whence);
		stream->Seek(position, direction);
		result = stream->Tell();
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occured while trying to seek file : %s\r\n", __FUNCTION__, except.what());
	}
	return result;
}

int32 CIoman::PreOpen(uint32 flags, const char* path)
{
	int32 handle = AllocateFileHandle();
	try
	{
		auto& file = m_files[handle];
		file.path = path;
		file.flags = flags;

		auto pathInfo = SplitPath(path);
		auto deviceIterator = m_devices.find(pathInfo.deviceName);
		auto userDeviceIterator = m_userDevices.find(pathInfo.deviceName);
		if(deviceIterator != m_devices.end())
		{
			file.stream = deviceIterator->second->GetFile(flags, pathInfo.devicePath.c_str());
			if(!file.stream)
			{
				throw FileNotFoundException();
			}
		}
		else if(userDeviceIterator != m_userDevices.end())
		{
			auto sysmem = m_bios.GetSysmem();
			file.descPtr = sysmem->AllocateMemory(sizeof(Ioman::DEVICEFILE), 0, 0);
			assert(file.descPtr != 0);
			auto desc = reinterpret_cast<Ioman::DEVICEFILE*>(m_ram + file.descPtr);
			desc->devicePtr = userDeviceIterator->second;
			desc->privateData = 0;
			desc->unit = 0;
			desc->mode = flags;
		}
		else
		{
			throw std::runtime_error("Unknown device.");
		}
	}
	catch(const CIoman::FileNotFoundException& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occurred while trying to open not existing file : %s\r\n", __FUNCTION__, path);
		FreeFileHandle(handle);

		return -ERROR_ENOENT;
	}
	catch(const std::exception& except)
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s: Error occurred while trying to open file : %s : %s\r\n", __FUNCTION__, path, except.what());
		FreeFileHandle(handle);

		return -1;
	}
	return handle;
}

Framework::STREAM_SEEK_DIRECTION CIoman::ConvertWhence(uint32 whence)
{
	switch(whence)
	{
	default:
		assert(false);
		[[fallthrough]];
	case SEEK_DIR_SET:
		return Framework::STREAM_SEEK_SET;
	case SEEK_DIR_CUR:
		return Framework::STREAM_SEEK_CUR;
	case SEEK_DIR_END:
		return Framework::STREAM_SEEK_END;
	}
}

bool CIoman::IsUserDeviceFileHandle(int32 fileHandle) const
{
	auto fileIterator = m_files.find(fileHandle);
	if(fileIterator == std::end(m_files)) return false;
	return GetUserDeviceFileDescPtr(fileHandle) != 0;
}

uint32 CIoman::GetUserDeviceFileDescPtr(int32 fileHandle) const
{
	auto fileIterator = m_files.find(fileHandle);
	assert(fileIterator != std::end(m_files));
	const auto& file = fileIterator->second;
	assert(!((file.descPtr != 0) && file.stream));
	return file.descPtr;
}

int32 CIoman::OpenVirtual(CMIPS& context)
{
	uint32 pathPtr = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 flags = context.m_State.nGPR[CMIPS::A1].nV0;

	auto path = reinterpret_cast<const char*>(m_ram + pathPtr);

	CLog::GetInstance().Print(LOG_NAME, "OpenVirtual(flags = 0x%08X, path = '%s');\r\n", flags, path);

	int32 handle = PreOpen(flags, path);
	if(handle < 0)
	{
		//PreOpen failed, bail
		return handle;
	}

	if(IsUserDeviceFileHandle(handle))
	{
		PrepareOpenThunk();

		auto devicePath = strchr(path, ':');
		assert(devicePath);
		auto devicePathPos = static_cast<uint32>(devicePath - path);
		uint32 descPtr = GetUserDeviceFileDescPtr(handle);
		auto desc = reinterpret_cast<Ioman::DEVICEFILE*>(m_ram + descPtr);
		auto device = reinterpret_cast<Ioman::DEVICE*>(m_ram + desc->devicePtr);
		auto ops = reinterpret_cast<Ioman::DEVICEOPS*>(m_ram + device->opsPtr);

		context.m_State.nPC = m_openThunkPtr;
		context.m_State.nGPR[CMIPS::A0].nV0 = descPtr;
		context.m_State.nGPR[CMIPS::A1].nV0 = pathPtr + devicePathPos + 1;
		context.m_State.nGPR[CMIPS::A2].nV0 = flags;
		context.m_State.nGPR[CMIPS::A3].nV0 = ops->openPtr;
		context.m_State.nGPR[CMIPS::T0].nV0 = handle;
		return 0;
	}

	return handle;
}

int32 CIoman::CloseVirtual(CMIPS& context)
{
	int32 handle = context.m_State.nGPR[CMIPS::A0].nV0;

	CLog::GetInstance().Print(LOG_NAME, "CloseVirtual(handle = %d);\r\n", handle);

	auto fileIterator = m_files.find(handle);
	if(fileIterator == std::end(m_files))
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s : Provided invalid fd %d.\r\n",
		                         __FUNCTION__, handle);
		return -1;
	}

	if(IsUserDeviceFileHandle(handle))
	{
		//TODO: Free file handle
		uint32 descPtr = GetUserDeviceFileDescPtr(handle);
		auto desc = reinterpret_cast<Ioman::DEVICEFILE*>(m_ram + descPtr);
		InvokeUserDeviceMethod(context, desc->devicePtr,
		                       offsetof(Ioman::DEVICEOPS, closePtr),
		                       descPtr);
		return 0;
	}
	else
	{
		return Close(handle);
	}
}

int32 CIoman::ReadVirtual(CMIPS& context)
{
	int32 handle = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 bufferPtr = context.m_State.nGPR[CMIPS::A1].nV0;
	uint32 count = context.m_State.nGPR[CMIPS::A2].nV0;

	CLog::GetInstance().Print(LOG_NAME, "ReadVirtual(handle = %d, size = 0x%X, buffer = ptr);\r\n", handle, count);

	auto fileIterator = m_files.find(handle);
	if(fileIterator == std::end(m_files))
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s : Provided invalid fd %d.\r\n",
		                         __FUNCTION__, handle);
		return -1;
	}

	if(IsUserDeviceFileHandle(handle))
	{
		uint32 descPtr = GetUserDeviceFileDescPtr(handle);
		auto desc = reinterpret_cast<Ioman::DEVICEFILE*>(m_ram + descPtr);
		InvokeUserDeviceMethod(context, desc->devicePtr,
		                       offsetof(Ioman::DEVICEOPS, readPtr),
		                       descPtr, bufferPtr, count);
		return 0;
	}
	else
	{
		return Read(handle, count, m_ram + bufferPtr);
	}
}

int32 CIoman::WriteVirtual(CMIPS& context)
{
	int32 handle = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 bufferPtr = context.m_State.nGPR[CMIPS::A1].nV0;
	uint32 count = context.m_State.nGPR[CMIPS::A2].nV0;

	CLog::GetInstance().Print(LOG_NAME, "WriteVirtual(handle = %d, size = 0x%X, buffer = ptr);\r\n", handle, count);

	auto fileIterator = m_files.find(handle);
	if(fileIterator == std::end(m_files))
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s : Provided invalid fd %d.\r\n",
								 __FUNCTION__, handle);
		return -1;
	}

	if(IsUserDeviceFileHandle(handle))
	{
		uint32 descPtr = GetUserDeviceFileDescPtr(handle);
		auto desc = reinterpret_cast<Ioman::DEVICEFILE*>(m_ram + descPtr);
		InvokeUserDeviceMethod(context, desc->devicePtr,
							   offsetof(Ioman::DEVICEOPS, writePtr),
							   descPtr, bufferPtr, count);
		return 0;
	}
	else
	{
		return Write(handle, count, m_ram + bufferPtr);
	}
}

int32 CIoman::SeekVirtual(CMIPS& context)
{
	int32 handle = context.m_State.nGPR[CMIPS::A0].nV0;
	uint32 position = context.m_State.nGPR[CMIPS::A1].nV0;
	uint32 whence = context.m_State.nGPR[CMIPS::A2].nV0;

	CLog::GetInstance().Print(LOG_NAME, "SeekVirtual(handle = %d, position = %d, whence = %d);\r\n",
	                          handle, position, whence);

	auto fileIterator = m_files.find(handle);
	if(fileIterator == std::end(m_files))
	{
		CLog::GetInstance().Warn(LOG_NAME, "%s : Provided invalid fd %d.\r\n",
		                         __FUNCTION__, handle);
		return -1;
	}

	if(IsUserDeviceFileHandle(handle))
	{
		uint32 descPtr = GetUserDeviceFileDescPtr(handle);
		auto desc = reinterpret_cast<Ioman::DEVICEFILE*>(m_ram + descPtr);
		InvokeUserDeviceMethod(context, desc->devicePtr,
		                       offsetof(Ioman::DEVICEOPS, lseekPtr),
		                       descPtr, position, whence);
		return 0;
	}
	else
	{
		return Seek(handle, position, whence);
	}
}

void CIoman::InvokeUserDeviceMethod(CMIPS& context, uint32 devicePtr, size_t opOffset, uint32 arg0, uint32 arg1, uint32 arg2)
{
	auto device = reinterpret_cast<Ioman::DEVICE*>(m_ram + devicePtr);
	auto opAddr = *reinterpret_cast<uint32*>(m_ram + device->opsPtr + opOffset);
	context.m_State.nGPR[CMIPS::A0].nV0 = arg0;
	context.m_State.nGPR[CMIPS::A1].nV0 = arg1;
	context.m_State.nGPR[CMIPS::A2].nV0 = arg2;
	context.m_State.nPC = opAddr;
}

int32 CIoman::AllocateFileHandle()
{
	uint32 handle = m_nextFileHandle++;
	assert(m_files.find(handle) == std::end(m_files));
	m_files[handle] = FileInfo();
	return handle;
}

void CIoman::FreeFileHandle(uint32 handle)
{
	assert(m_files.find(handle) != std::end(m_files));
	m_files.erase(handle);
}

uint32 CIoman::GetFileMode(uint32 handle) const
{
	auto file(m_files.find(handle));
	if(file == std::end(m_files))
	{
		throw std::runtime_error("Invalid file handle.");
	}
	return file->second.flags;
}

Framework::CStream* CIoman::GetFileStream(uint32 handle)
{
	auto file(m_files.find(handle));
	if(file == std::end(m_files))
	{
		throw std::runtime_error("Invalid file handle.");
	}
	return file->second.stream;
}

void CIoman::SetFileStream(uint32 handle, Framework::CStream* stream)
{
	m_files.erase(handle);
	m_files[handle] = {stream};
}

//IOP Invoke
void CIoman::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(OpenVirtual(context));
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(CloseVirtual(context));
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(ReadVirtual(context));
		break;
	case 7:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(WriteVirtual(context));
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(SeekVirtual(context));
		break;
	case 16:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(GetStat(
		    reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV[0]]),
		    reinterpret_cast<Ioman::STAT*>(&m_ram[context.m_State.nGPR[CMIPS::A1].nV[0]])));
		break;
	case 20:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(AddDrv(context));
		break;
	case 21:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DelDrv(
		    context.m_State.nGPR[CMIPS::A0].nV0));
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "%s(%08X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
		break;
	}
}

void CIoman::SaveState(Framework::CZipArchiveWriter& archive) const
{
	SaveFilesState(archive);
	SaveUserDevicesState(archive);
}

void CIoman::LoadState(Framework::CZipArchiveReader& archive)
{
	LoadFilesState(archive);
	LoadUserDevicesState(archive);
}

void CIoman::SaveFilesState(Framework::CZipArchiveWriter& archive) const
{
	auto fileStateFile = new CXmlStateFile(STATE_FILES_FILENAME, STATE_FILES_FILESNODE);
	auto filesStateNode = fileStateFile->GetRoot();

	for(const auto& filePair : m_files)
	{
		if(filePair.first == FID_STDOUT) continue;
		if(filePair.first == FID_STDERR) continue;

		const auto& file = filePair.second;

		auto fileStateNode = new Framework::Xml::CNode(STATE_FILES_FILENODE, true);
		fileStateNode->InsertAttribute(Framework::Xml::CreateAttributeIntValue(STATE_FILES_FILENODE_IDATTRIBUTE, filePair.first));
		fileStateNode->InsertAttribute(Framework::Xml::CreateAttributeIntValue(STATE_FILES_FILENODE_FLAGSATTRIBUTE, file.flags));
		fileStateNode->InsertAttribute(Framework::Xml::CreateAttributeIntValue(STATE_FILES_FILENODE_DESCPTRATTRIBUTE, file.descPtr));
		fileStateNode->InsertAttribute(Framework::Xml::CreateAttributeStringValue(STATE_FILES_FILENODE_PATHATTRIBUTE, file.path.c_str()));
		filesStateNode->InsertNode(fileStateNode);
	}

	archive.InsertFile(fileStateFile);
}

void CIoman::SaveUserDevicesState(Framework::CZipArchiveWriter& archive) const
{
	auto deviceStateFile = new CXmlStateFile(STATE_USERDEVICES_FILENAME, STATE_USERDEVICES_DEVICESNODE);
	auto devicesStateNode = deviceStateFile->GetRoot();

	for(const auto& devicePair : m_userDevices)
	{
		auto deviceStateNode = new Framework::Xml::CNode(STATE_USERDEVICES_DEVICENODE, true);
		deviceStateNode->InsertAttribute(Framework::Xml::CreateAttributeStringValue(STATE_USERDEVICES_DEVICENODE_NAMEATTRIBUTE, devicePair.first.c_str()));
		deviceStateNode->InsertAttribute(Framework::Xml::CreateAttributeIntValue(STATE_USERDEVICES_DEVICENODE_DESCPTRATTRIBUTE, devicePair.second));
		devicesStateNode->InsertNode(deviceStateNode);
	}

	archive.InsertFile(deviceStateFile);
}

void CIoman::LoadFilesState(Framework::CZipArchiveReader& archive)
{
	std::experimental::erase_if(m_files,
	                            [](const FileMapType::value_type& filePair) {
		                            return (filePair.first != FID_STDOUT) && (filePair.first != FID_STDERR);
	                            });

	auto fileStateFile = CXmlStateFile(*archive.BeginReadFile(STATE_FILES_FILENAME));
	auto fileStateNode = fileStateFile.GetRoot();

	int32 maxFileId = FID_STDERR;
	auto fileNodes = fileStateNode->SelectNodes(STATE_FILES_FILESNODE "/" STATE_FILES_FILENODE);
	for(auto fileNode : fileNodes)
	{
		int32 id = 0, flags = 0, descPtr = 0;
		std::string path;
		if(!Framework::Xml::GetAttributeIntValue(fileNode, STATE_FILES_FILENODE_IDATTRIBUTE, &id)) break;
		if(!Framework::Xml::GetAttributeStringValue(fileNode, STATE_FILES_FILENODE_PATHATTRIBUTE, &path)) break;
		if(!Framework::Xml::GetAttributeIntValue(fileNode, STATE_FILES_FILENODE_FLAGSATTRIBUTE, &flags)) break;
		if(!Framework::Xml::GetAttributeIntValue(fileNode, STATE_FILES_FILENODE_DESCPTRATTRIBUTE, &descPtr)) break;

		FileInfo fileInfo;
		fileInfo.flags = flags;
		fileInfo.path = path;
		fileInfo.descPtr = descPtr;
		fileInfo.stream = (descPtr == 0) ? OpenInternal(flags, path.c_str()) : nullptr;
		m_files[id] = std::move(fileInfo);

		maxFileId = std::max(maxFileId, id);
	}
	m_nextFileHandle = maxFileId + 1;
}

void CIoman::LoadUserDevicesState(Framework::CZipArchiveReader& archive)
{
	m_userDevices.clear();

	auto deviceStateFile = CXmlStateFile(*archive.BeginReadFile(STATE_USERDEVICES_FILENAME));
	auto deviceStateNode = deviceStateFile.GetRoot();

	auto deviceNodes = deviceStateNode->SelectNodes(STATE_USERDEVICES_DEVICESNODE "/" STATE_USERDEVICES_DEVICENODE);
	for(auto deviceNode : deviceNodes)
	{
		std::string name;
		int32 descPtr = 0;
		if(!Framework::Xml::GetAttributeStringValue(deviceNode, STATE_USERDEVICES_DEVICENODE_NAMEATTRIBUTE, &name)) break;
		if(!Framework::Xml::GetAttributeIntValue(deviceNode, STATE_USERDEVICES_DEVICENODE_DESCPTRATTRIBUTE, &descPtr)) break;

		m_userDevices[name] = descPtr;
	}
}
