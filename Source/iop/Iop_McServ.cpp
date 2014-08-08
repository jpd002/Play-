#include <assert.h>
#include <stdio.h>
#include <boost/filesystem/operations.hpp>
#include "../AppConfig.h"
#include "../Log.h"
#include "Iop_McServ.h"

using namespace Iop;
namespace filesystem = boost::filesystem;

#define LOG_NAME ("iop_mcserv")

const char* CMcServ::m_mcPathPreference[2] =
{
	"ps2.mc0.directory",
	"ps2.mc1.directory",
};

CMcServ::CMcServ(CSifMan& sif)
{
	sif.RegisterModule(MODULE_ID, this);
}

CMcServ::~CMcServ()
{

}

std::string CMcServ::GetId() const
{
	return "mcserv";
}

std::string CMcServ::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CMcServ::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CMcServ::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x01:
		GetInfo(args, argsSize, ret, retSize, ram);
		break;
	case 0x02:
		Open(args, argsSize, ret, retSize, ram);
		break;
	case 0x03:
		Close(args, argsSize, ret, retSize, ram);
		break;
	case 0x04:
		Seek(args, argsSize, ret, retSize, ram);
		break;
	case 0x05:
		Read(args, argsSize, ret, retSize, ram);
		break;
	case 0x06:
		Write(args, argsSize, ret, retSize, ram);
		break;
	case 0x0A:
		Flush(args, argsSize, ret, retSize, ram);
		break;
	case 0x0C:
		ChDir(args, argsSize, ret, retSize, ram);
		break;
	case 0x0D:
		GetDir(args, argsSize, ret, retSize, ram);
		break;
	case 0xFE:
		//Get version?
		GetVersionInformation(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X).\r\n", method);
		break;
	}
	return true;
}

void CMcServ::GetInfo(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x1C);

	uint32 port			= args[1];
	uint32 slot			= args[2];
	bool wantType		= args[3] != 0;
	bool wantFreeSpace	= args[4] != 0;
	bool wantFormatted	= args[5] != 0;
	uint32* retBuffer	= reinterpret_cast<uint32*>(&ram[args[7]]);

	CLog::GetInstance().Print(LOG_NAME, "GetInfo(port = %i, slot = %i, wantType = %i, wantFreeSpace = %i, wantFormatted = %i, retBuffer = 0x%0.8X);\r\n",
		port, slot, wantType, wantFreeSpace, wantFormatted, args[7]);

	if(wantType)
	{
		retBuffer[0x00] = 2;
	}
	if(wantFreeSpace)
	{
		retBuffer[0x01] = 0x800000;
	}
	if(wantFormatted)
	{
		retBuffer[0x24] = 1;
	}

	//Return values
	//  0 if same card as previous call
	//  -1 if new formatted card
	//  -2 if new unformatted card
	//> -2 on error
	ret[0] = 0;
}

void CMcServ::Open(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x414);

	CMD* cmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Open(port = %i, slot = %i, flags = %i, name = %s);\r\n",
		cmd->port, cmd->slot, cmd->flags, cmd->name);

	if(cmd->port > 1)
	{
		assert(0);
		ret[0] = -1;
		return;
	}

	filesystem::path filePath;

	try
	{
		filesystem::path mcPath = filesystem::path(CAppConfig::GetInstance().GetPreferenceString(m_mcPathPreference[cmd->port]));
		filesystem::path requestedFilePath(cmd->name);

		if(!requestedFilePath.root_directory().empty())
		{
			filePath = mcPath / requestedFilePath;
		}
		else
		{
			filePath = mcPath / m_currentDirectory / requestedFilePath;
		}
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Print(LOG_NAME, "Error while executing Open: %s\r\n.", exception.what());
		ret[0] = -1;
		return;
	}

	if(cmd->flags == 0x40)
	{
		//Directory only?
		uint32 result = -1;
		try
		{
			filesystem::create_directory(filePath);
			result = 0;
		}
		catch(...)
		{
			
		}
		ret[0] = result;
		return;
	}
	else
	{
		const char* access = nullptr;
		switch(cmd->flags)
		{
		case OPEN_FLAG_RDONLY:
			access = "rb";
			break;
		case OPEN_FLAG_WRONLY:
			access = "r+b";
			break;
		case (OPEN_FLAG_CREAT | OPEN_FLAG_WRONLY):
		case (OPEN_FLAG_CREAT | OPEN_FLAG_RDWR):
			access = "wb";
			break;
		}

		if(access == nullptr)
		{
			ret[0] = -1;
			assert(0);
			return;
		}

		try
		{
			auto file = Framework::CStdStream(filePath.string().c_str(), access);
			uint32 handle = GenerateHandle();
			if(handle == -1)
			{
				//Exhausted all file handles
				throw std::exception();
			}
			m_files[handle] = std::move(file);
			ret[0] = handle;
		}
		catch(...)
		{
			//-4 for not existing file?
			ret[0] = -4;
			return;
		}
	}
}

void CMcServ::Close(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Close(handle = %i);\r\n", cmd->handle);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	file->Clear();

	ret[0] = 0;
}

void CMcServ::Seek(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Seek(handle = %i, offset = 0x%0.8X, origin = 0x%0.8X);\r\n",
		cmd->handle, cmd->offset, cmd->origin);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	Framework::STREAM_SEEK_DIRECTION origin = Framework::STREAM_SEEK_SET;
	switch(cmd->origin)
	{
	case 0:
		origin = Framework::STREAM_SEEK_SET;
		break;
	case 1:
		origin = Framework::STREAM_SEEK_CUR;
		break;
	case 2:
		origin = Framework::STREAM_SEEK_END;
		break;
	default:
		assert(0);
		break;
	}

	file->Seek(cmd->offset, origin);
	ret[0] = static_cast<uint32>(file->Tell());
}

void CMcServ::Read(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Read(handle = %i, size = 0x%0.8X, bufferAddress = 0x%0.8X, paramAddress = 0x%0.8X);\r\n",
		cmd->handle, cmd->size, cmd->bufferAddress, cmd->paramAddress);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	assert(cmd->bufferAddress != 0);
	void* dst = &ram[cmd->bufferAddress];

	if(cmd->paramAddress != 0)
	{
		//This param buffer is used in the callback after calling this method... No clue what it's for
		reinterpret_cast<uint32*>(&ram[cmd->paramAddress])[0] = 0;
		reinterpret_cast<uint32*>(&ram[cmd->paramAddress])[1] = 0;
	}

	ret[0] = static_cast<uint32>(file->Read(dst, cmd->size));
}

void CMcServ::Write(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Write(handle = %i, nSize = 0x%0.8X, bufferAddress = 0x%0.8X, origin = 0x%0.8X);\r\n",
		cmd->handle, cmd->size, cmd->bufferAddress, cmd->origin);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	const void* dst = &ram[cmd->bufferAddress];
	uint32 result = 0;

	//Write "origin" bytes from "data" field first
	if(cmd->origin != 0)
	{
		file->Write(cmd->data, cmd->origin);
		result += cmd->origin;
	}

	result += static_cast<uint32>(file->Write(dst, cmd->size));
	ret[0] = result;
}

void CMcServ::Flush(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* cmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Flush(handle = %d);\r\n", cmd->handle);

	auto file = GetFileFromHandle(cmd->handle);
	if(file == nullptr)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	file->Flush();

	ret[0] = 0;
}

void CMcServ::ChDir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x414);
	CMD* cmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "ChDir(port = %i, slot = %i, tableAddress = 0x%0.8X, name = %s);\r\n",
							  cmd->port, cmd->slot, cmd->tableAddress, cmd->name);

	uint32 result = -1;

	try
	{
		filesystem::path newCurrentDirectory;
		filesystem::path requestedDirectory(cmd->name);

		if(!requestedDirectory.root_directory().empty())
		{
			newCurrentDirectory = requestedDirectory;
		}
		else
		{
			newCurrentDirectory = m_currentDirectory / requestedDirectory;
		}

		filesystem::path mcPath(CAppConfig::GetInstance().GetPreferenceString(m_mcPathPreference[cmd->port]));
		mcPath /= newCurrentDirectory;

		if(filesystem::exists(mcPath) && filesystem::is_directory(mcPath))
		{
			m_currentDirectory = newCurrentDirectory;
			result = 0;
		}
		else
		{
			//Not found (I guess)
			result = -4;
		}
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Print(LOG_NAME, "Error while executing GetDir: %s\r\n.", exception.what());
	}

	ret[0] = result;
}

void CMcServ::GetDir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 result = 0;

	assert(argsSize >= 0x414);

	CMD* cmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "GetDir(port = %i, slot = %i, flags = %i, maxEntries = %i, tableAddress = 0x%0.8X, name = %s);\r\n",
		cmd->port, cmd->slot, cmd->flags, cmd->maxEntries, cmd->tableAddress, cmd->name);

	if(cmd->port > 1)
	{
		assert(0);
		ret[0] = -1;
		return;
	}

	try
	{
		if(cmd->flags == 0)
		{
			m_pathFinder.Reset();

			filesystem::path mcPath(CAppConfig::GetInstance().GetPreferenceString(m_mcPathPreference[cmd->port]));
			mcPath /= m_currentDirectory;
			mcPath = filesystem::absolute(mcPath);

			if(filesystem::exists(mcPath))
			{
				m_pathFinder.Search(mcPath, cmd->name);
			}
		}

		if(cmd->maxEntries > 0)
		{
			result = m_pathFinder.Read(reinterpret_cast<ENTRY*>(&ram[cmd->tableAddress]), cmd->maxEntries);
		}
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Print(LOG_NAME, "Error while executing GetDir: %s\r\n.", exception.what());
	}

	ret[0] = result;
}

void CMcServ::GetVersionInformation(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize == 0x30);
	assert(retSize == 0x0C);

	ret[0] = 0x00000000;
	ret[1] = 0x0000020A;		//mcserv version
	ret[2] = 0x0000020E;		//mcman version

	CLog::GetInstance().Print(LOG_NAME, "Init();\r\n");
}

uint32 CMcServ::GenerateHandle()
{
	for(unsigned int i = 0; i < MAX_FILES; i++)
	{
		if(m_files[i].IsEmpty()) return i;
	}
	return -1;
}

Framework::CStdStream* CMcServ::GetFileFromHandle(uint32 handle)
{
	assert(handle < MAX_FILES);
	if(handle >= MAX_FILES)
	{
		return nullptr;
	}
	auto& file = m_files[handle];
	if(file.IsEmpty())
	{
		return nullptr;
	}
	return &file;
}

/////////////////////////////////////////////
//CPathFinder Implementation
/////////////////////////////////////////////

CMcServ::CPathFinder::CPathFinder()
: m_index(0)
{

}

CMcServ::CPathFinder::~CPathFinder()
{
	
}

void CMcServ::CPathFinder::Reset()
{
	m_entries.clear();
	m_index = 0;
}

void CMcServ::CPathFinder::Search(const boost::filesystem::path& basePath, const char* filter)
{
	m_basePath	= basePath;
	m_filter	= filter;

	if(m_filter[0] != '/')
	{
		m_filter = "/" + m_filter;
	}
	if(m_filter.find('?') != std::string::npos)
	{
		m_matcher = &CPathFinder::QuestionMarkFilterMatcher;
	}
	else
	{
		m_matcher = &CPathFinder::StarFilterMatcher;
	}

	SearchRecurse(m_basePath);
}

unsigned int CMcServ::CPathFinder::Read(ENTRY* entry, unsigned int size)
{
	assert(m_index <= m_entries.size());
	unsigned int remaining = m_entries.size() - m_index;
	unsigned int readCount = std::min<unsigned int>(remaining, size);
	for(unsigned int i = 0; i < readCount; i++)
	{
		entry[i] = m_entries[i + m_index];
	}
	m_index += readCount;
	return readCount;
}

void CMcServ::CPathFinder::SearchRecurse(const filesystem::path& path)
{
	filesystem::directory_iterator endIterator;

	for(filesystem::directory_iterator elementIterator(path);
		elementIterator != endIterator; elementIterator++)
	{
		boost::filesystem::path relativePath(*elementIterator);
		std::string relativePathString(relativePath.generic_string());

		//"Extract" a more appropriate relative path from the memory card point of view
		relativePathString.erase(0, m_basePath.string().size());

		//Attempt to match this against the filter
		if((*this.*m_matcher)(relativePathString.c_str()))
		{
			//Fill in the information
			ENTRY entry;
			memset(&entry, 0, sizeof(entry));

			strncpy(reinterpret_cast<char*>(entry.name), relativePath.filename().string().c_str(), 0x1F);
			entry.name[0x1F] = 0;

			if(filesystem::is_directory(*elementIterator))
			{
				entry.size			= 0;
				entry.attributes	= 0x8427;
			}
			else
			{
				entry.size			= static_cast<uint32>(filesystem::file_size(*elementIterator));
				entry.attributes	= 0x8497;
			}

			//Fill in modification date info
			{
				auto changeDate = filesystem::last_write_time(*elementIterator);
				auto localChangeDate = localtime(&changeDate);

				entry.modificationTime.second = localChangeDate->tm_sec;
				entry.modificationTime.minute = localChangeDate->tm_min;
				entry.modificationTime.hour = localChangeDate->tm_hour;
				entry.modificationTime.day = localChangeDate->tm_mday;
				entry.modificationTime.month = localChangeDate->tm_mon;
				entry.modificationTime.year = localChangeDate->tm_year + 1900;
			}

			//boost::filesystem doesn't provide a way to get creation time, so just make it the same as modification date
			entry.creationTime = entry.modificationTime;

			m_entries.push_back(entry);
		}

		if(filesystem::is_directory(*elementIterator))
		{
			SearchRecurse(*elementIterator);
		}
	}
}

//Based on an algorithm found on http://xoomer.alice.it/acantato/dev/wildcard/wildmatch.html
bool CMcServ::CPathFinder::StarFilterMatcher(const char* path)
{
	const char* pattern = m_filter.c_str();
	bool star = false;

	const char* s(nullptr);
	const char* p(nullptr);

_loopStart:

	for(s = path, p = pattern; *s; s++, p++)
	{
		switch(*p)
		{
		case '*':
			star = true;
			path = s;
			pattern = p;
			if((*++p) == 0) return true;
			goto _loopStart;
			break;
		default:
			if(toupper(*s) != toupper(*p))
			{
				goto _starCheck;
			}
			break;
		}
	}

	if(*p == '*') p++;
	return (*p) == 0;

_starCheck:
	if(!star) return false;
	path++;
	goto _loopStart;
}

bool CMcServ::CPathFinder::QuestionMarkFilterMatcher(const char* path)
{
	const char* src = m_filter.c_str();
	const char* dst = path;

	while(1)
	{
		if(*src == 0 && *dst == 0)
		{
			return true;
		}

		if(*src == 0) return false;
		if(*dst == 0) return false;

		if(*src != '?')
		{
			if(*src != *dst) return false;
		}

		src++;
		dst++;
	}

	return false;
}
