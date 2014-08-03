#include "../AppConfig.h"
#include "Iop_Ioman.h"
#include "StdStream.h"
#include <stdexcept>

using namespace Iop;

#define PREF_IOP_FILEIO_STDLOGGING ("iop.fileio.stdlogging")

#define FID_STDOUT	(1)
#define FID_STDERR	(2)

CIoman::CIoman(uint8* ram) 
: m_ram(ram)
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

			m_files[FID_STDOUT] = new Framework::CStdStream(fopen(stdoutPath.string().c_str(), "ab"));
			m_files[FID_STDERR] = new Framework::CStdStream(fopen(stderrPath.string().c_str(), "ab"));
		}
		catch(...)
		{
			//Humm, some error occured when opening these files...
		}
	}
}

CIoman::~CIoman()
{
	for(auto fileIterator(std::begin(m_files));
		std::end(m_files) != fileIterator; fileIterator++)
	{
		delete fileIterator->second;
	}
	m_devices.clear();
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
	case 6:
		return "read";
		break;
	case 8:
		return "seek";
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
	uint32 handle = 0xFFFFFFFF;
	try
	{
		std::string fullPath(path);
		std::string::size_type position = fullPath.find(":");
		if(position == std::string::npos) 
		{
			throw std::runtime_error("Invalid path.");
		}
		std::string deviceName(fullPath.begin(), fullPath.begin() + position);
		std::string devicePath(fullPath.begin() + position + 1, fullPath.end());
		DeviceMapType::iterator device(m_devices.find(deviceName));
		if(device == m_devices.end())
		{
			throw std::runtime_error("Device not found.");
		}
		Framework::CStream* stream = device->second->GetFile(flags, devicePath.c_str());
		if(stream == NULL)
		{
			throw std::runtime_error("File not found.");
		}
		handle = m_nextFileHandle++;
		m_files[handle] = stream;
	}
	catch(const std::exception& except)
	{
		printf("%s: Error occured while trying to open file : %s\r\n", __FUNCTION__, except.what());
	}
	return handle;
}

uint32 CIoman::Close(uint32 handle)
{
	uint32 result = 0xFFFFFFFF;
	try
	{
		auto file(m_files.find(handle));
		if(file == std::end(m_files))
		{
			throw std::runtime_error("Invalid file handle.");
		}
		delete file->second;
		m_files.erase(file);
		result = 0;
	}
	catch(const std::exception& except)
	{
		printf("%s: Error occured while trying to close file : %s\r\n", __FUNCTION__, except.what());
	}
	return result;
}

uint32 CIoman::Read(uint32 handle, uint32 size, void* buffer)
{
	uint32 result = 0xFFFFFFFF;
	try
	{
		Framework::CStream* stream = GetFileStream(handle);
		result = static_cast<uint32>(stream->Read(buffer, size));
	}
	catch(const std::exception& except)
	{
		printf("%s: Error occured while trying to read file : %s\r\n", __FUNCTION__, except.what());
	}
	return result;
}

uint32 CIoman::Write(uint32 handle, uint32 size, const void* buffer)
{
	uint32 result = 0xFFFFFFFF;
	try
	{
		Framework::CStream* stream = GetFileStream(handle);
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
			printf("%s: Error occured while trying to write file : %s\r\n", __FUNCTION__, except.what());
		}
	}
	return result;
}

uint32 CIoman::Seek(uint32 handle, uint32 position, uint32 whence)
{
	uint32 result = 0xFFFFFFFF;
	try
	{
		Framework::CStream* stream = GetFileStream(handle);
		switch(whence)
		{
		case 0:
			whence = Framework::STREAM_SEEK_SET;
			break;
		case 1:
			whence = Framework::STREAM_SEEK_CUR;
			break;
		case 2:
			whence = Framework::STREAM_SEEK_END;
			break;
		}

		stream->Seek(position, static_cast<Framework::STREAM_SEEK_DIRECTION>(whence));
		result = static_cast<uint32>(stream->Tell());
	}
	catch(const std::exception& except)
	{
		printf("%s: Error occured while trying to seek file : %s\r\n", __FUNCTION__, except.what());
	}
	return result;
}

uint32 CIoman::AddDrv(uint32 drvPtr)
{
	return -1;
}

uint32 CIoman::DelDrv(const char* deviceName)
{
	return -1;
}

Framework::CStream* CIoman::GetFileStream(uint32 handle)
{
	auto file(m_files.find(handle));
	if(file == std::end(m_files))
	{
		throw std::runtime_error("Invalid file handle.");
	}
	return file->second;
}

//IOP Invoke
void CIoman::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 4:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Open(
			context.m_State.nGPR[CMIPS::A1].nV[0],
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nV[0]])
			));
		break;
	case 5:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Close(
			context.m_State.nGPR[CMIPS::A0].nV[0]
			));
		break;
	case 6:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Read(
			context.m_State.nGPR[CMIPS::A0].nV[0],
			context.m_State.nGPR[CMIPS::A2].nV[0],
			&m_ram[context.m_State.nGPR[CMIPS::A1].nV[0]]
			));
		break;
	case 8:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(Seek(
			context.m_State.nGPR[CMIPS::A0].nV[0],
			context.m_State.nGPR[CMIPS::A1].nV[0],
			context.m_State.nGPR[CMIPS::A2].nV[0]));
		break;
	case 20:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(AddDrv(
			context.m_State.nGPR[CMIPS::A0].nV0
		));
		break;
	case 21:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(DelDrv(
			reinterpret_cast<char*>(&m_ram[context.m_State.nGPR[CMIPS::A0].nD0])
		));
		break;
	default:
		printf("%s(%0.8X): Unknown function (%d) called.\r\n", __FUNCTION__, context.m_State.nPC, functionId);
		break;
	}
}

//--------------------------------------------------
// CFile
//--------------------------------------------------

CIoman::CFile::CFile(uint32 handle, CIoman& ioman)
: m_handle(handle)
, m_ioman(ioman)
{

}

CIoman::CFile::~CFile()
{
	m_ioman.Close(m_handle);
}

CIoman::CFile::operator uint32()
{
	return m_handle;
}
