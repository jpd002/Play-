#include <assert.h>
#include <stdio.h>
#include <boost/filesystem/operations.hpp>
#include "../AppConfig.h"
#include "../Log.h"
#include "Iop_McServ.h"

using namespace Iop;
namespace filesystem = boost::filesystem;

#define LOG_NAME ("iop_mcserv")

const char* CMcServ::m_sMcPathPreference[2] =
{
	"ps2.mc0.directory",
	"ps2.mc1.directory",
};

CMcServ::CMcServ(CSifMan& sif)
: m_nNextHandle(1)
{
	sif.RegisterModule(MODULE_ID, this);
}

CMcServ::~CMcServ()
{
	//Close any handles that might still be in there...
	for(HandleMap::iterator itHandle = m_Handles.begin();
		itHandle != m_Handles.end();
		itHandle++)
	{
		fclose(itHandle->second);
	}
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

	uint32 nPort			= args[1];
	uint32 nSlot			= args[2];
	bool nWantType			= args[3] != 0;
	bool nWantFreeSpace		= args[4] != 0;
	bool nWantFormatted		= args[5] != 0;
	uint32* pRetBuffer		= reinterpret_cast<uint32*>(&ram[args[7]]);

	CLog::GetInstance().Print(LOG_NAME, "GetInfo(nPort = %i, nSlot = %i, nWantType = %i, nWantFreeSpace = %i, nWantFormatted = %i, nRetBuffer = 0x%0.8X);\r\n",
		nPort, nSlot, nWantType, nWantFreeSpace, nWantFormatted, args[7]);

	if(nWantType)
	{
		pRetBuffer[0x00] = 2;
	}
	if(nWantFreeSpace)
	{
		pRetBuffer[0x01] = 0x800000;
	}
	if(nWantFormatted)
	{
		pRetBuffer[0x24] = 1;
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

	CMD* pCmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Open(nPort = %i, nSlot = %i, nFlags = %i, sName = %s);\r\n",
		pCmd->nPort, pCmd->nSlot, pCmd->nFlags, pCmd->sName);

	if(pCmd->nPort > 1)
	{
		assert(0);
		ret[0] = -1;
		return;
	}

	filesystem::path filePath;

	try
	{
		filesystem::path mcPath = filesystem::path(CAppConfig::GetInstance().GetPreferenceString(m_sMcPathPreference[pCmd->nPort]));
		filesystem::path requestedFilePath(pCmd->sName);

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

	if(pCmd->nFlags == 0x40)
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
		const char* sAccess = NULL;
		switch(pCmd->nFlags)
		{
		case OPEN_FLAG_RDONLY:
			sAccess = "rb";
			break;
		case OPEN_FLAG_WRONLY:
			sAccess = "r+b";
			break;
		case (OPEN_FLAG_CREAT | OPEN_FLAG_WRONLY):
		case (OPEN_FLAG_CREAT | OPEN_FLAG_RDWR):
			sAccess = "wb";
			break;
		}

		if(sAccess == NULL)
		{
			ret[0] = -1;
			assert(0);
			return;
		}

		FILE* pFile = fopen(filePath.string().c_str(), sAccess);
		if(pFile == NULL)
		{
			//-4 for not existing file?
			ret[0] = -4;
			return;
		}

		uint32 nHandle = GenerateHandle();
		m_Handles[nHandle] = pFile;

		ret[0] = nHandle;
	}
}

void CMcServ::Close(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* pCmd;

	pCmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Close(nHandle = %i);\r\n",
		pCmd->nHandle);

	HandleMap::iterator itHandle;

	itHandle = m_Handles.find(pCmd->nHandle);
	if(itHandle == m_Handles.end())
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	fclose(itHandle->second);
	m_Handles.erase(itHandle);

	ret[0] = 0;
}

void CMcServ::Seek(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* pCmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Seek(nHandle = %i, nOffset = 0x%0.8X, nOrigin = 0x%0.8X);\r\n",
		pCmd->nHandle, pCmd->nOffset, pCmd->nOrigin);

	FILE* pFile = GetFileFromHandle(pCmd->nHandle);
	if(pFile == NULL)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	int origin = SEEK_SET;
	switch(pCmd->nOrigin)
	{
	case 0:
		origin = SEEK_SET;
		break;
	case 1:
		origin = SEEK_CUR;
		break;
	case 2:
		origin = SEEK_END;
		break;
	default:
		assert(0);
		break;
	}

	fseek(pFile, pCmd->nOffset, origin);
	ret[0] = ftell(pFile);
}

void CMcServ::Read(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* pCmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Read(nHandle = %i, nSize = 0x%0.8X, nBufferAddress = 0x%0.8X, nParamAddress = 0x%0.8X);\r\n",
		pCmd->nHandle, pCmd->nSize, pCmd->nBufferAddress, pCmd->nParamAddress);

	FILE* pFile = GetFileFromHandle(pCmd->nHandle);
	if(pFile == NULL)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	assert(pCmd->nBufferAddress != 0);
	void* pDst = &ram[pCmd->nBufferAddress];

	if(pCmd->nParamAddress != 0)
	{
		//This param buffer is used in the callback after calling this method... No clue what it's for
		reinterpret_cast<uint32*>(&ram[pCmd->nParamAddress])[0] = 0;
		reinterpret_cast<uint32*>(&ram[pCmd->nParamAddress])[1] = 0;
	}

	ret[0] = static_cast<uint32>(fread(pDst, 1, pCmd->nSize, pFile));
}

void CMcServ::Write(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	FILECMD* pCmd = reinterpret_cast<FILECMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "Write(nHandle = %i, nSize = 0x%0.8X, nBufferAddress = 0x%0.8X, nOrigin = 0x%0.8X);\r\n",
		pCmd->nHandle, pCmd->nSize, pCmd->nBufferAddress, pCmd->nOrigin);

	FILE* pFile = GetFileFromHandle(pCmd->nHandle);
	if(pFile == NULL)
	{
		ret[0] = -1;
		assert(0);
		return;
	}

	void* pDst = &ram[pCmd->nBufferAddress];
	uint32 result = 0;

	//Write "origin" bytes from "data" field first
	if(pCmd->nOrigin != 0)
	{
		fwrite(pCmd->nData, 1, pCmd->nOrigin, pFile);
		result += pCmd->nOrigin;
	}

	result += static_cast<uint32>(fwrite(pDst, 1, pCmd->nSize, pFile));
	ret[0] = result;
}

void CMcServ::ChDir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x414);
	CMD* pCmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "ChDir(nPort = %i, nSlot = %i, pTable = 0x%0.8X, pName = %s);\r\n",
							  pCmd->nPort, pCmd->nSlot, pCmd->nTableAddress, pCmd->sName);

	uint32 nRet = -1;

	try
	{
		filesystem::path newCurrentDirectory;
		filesystem::path requestedDirectory(pCmd->sName);

		if(!requestedDirectory.root_directory().empty())
		{
			newCurrentDirectory = requestedDirectory;
		}
		else
		{
			newCurrentDirectory = m_currentDirectory / requestedDirectory;
		}

		filesystem::path mcPath(CAppConfig::GetInstance().GetPreferenceString(m_sMcPathPreference[pCmd->nPort]));
		mcPath /= newCurrentDirectory;

		if(filesystem::exists(mcPath) && filesystem::is_directory(mcPath))
		{
			m_currentDirectory = newCurrentDirectory;
			nRet = 0;
		}
		else
		{
			//Not found (I guess)
			nRet = -4;
		}
	}
	catch(const std::exception& Exception)
	{
		CLog::GetInstance().Print(LOG_NAME, "Error while executing GetDir: %s\r\n.", Exception.what());
	}

	ret[0] = nRet;
}

void CMcServ::GetDir(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nRet = 0;

	assert(argsSize >= 0x414);

	CMD* pCmd = reinterpret_cast<CMD*>(args);

	CLog::GetInstance().Print(LOG_NAME, "GetDir(nPort = %i, nSlot = %i, nFlags = %i, nMaxEntries = %i, nTableAddress = 0x%0.8X, sName = %s);\r\n",
		pCmd->nPort, pCmd->nSlot, pCmd->nFlags, pCmd->nMaxEntries, pCmd->nTableAddress, pCmd->sName);

	if(pCmd->nPort > 1)
	{
		assert(0);
		ret[0] = -1;
		return;
	}

	try
	{
		if(pCmd->nFlags == 0)
		{
			m_pathFinder.Reset();

			filesystem::path mcPath(CAppConfig::GetInstance().GetPreferenceString(m_sMcPathPreference[pCmd->nPort]));
			mcPath /= m_currentDirectory;
			mcPath = filesystem::absolute(mcPath);

			if(filesystem::exists(mcPath))
			{
				m_pathFinder.Search(mcPath, pCmd->sName);
			}
		}

		nRet = m_pathFinder.Read(reinterpret_cast<ENTRY*>(&ram[pCmd->nTableAddress]), pCmd->nMaxEntries);
	}
	catch(const std::exception& Exception)
	{
		CLog::GetInstance().Print(LOG_NAME, "Error while executing GetDir: %s\r\n.", Exception.what());
	}

	ret[0] = nRet;
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
	return m_nNextHandle++;
}

FILE* CMcServ::GetFileFromHandle(uint32 handle)
{
	HandleMap::iterator itHandle = m_Handles.find(handle);
	if(itHandle == m_Handles.end())
	{
		return NULL;
	}
	return itHandle->second;
}

/////////////////////////////////////////////
//CPathFinder Implementation
/////////////////////////////////////////////

CMcServ::CPathFinder::CPathFinder()
: m_nIndex(0)
{

}

CMcServ::CPathFinder::~CPathFinder()
{
	
}

void CMcServ::CPathFinder::Reset()
{
	m_entries.clear();
	m_nIndex = 0;
}

void CMcServ::CPathFinder::Search(const boost::filesystem::path& BasePath, const char* sFilter)
{
	m_BasePath	= BasePath;
	m_sFilter	= sFilter;

	if(m_sFilter[0] != '/')
	{
		m_sFilter = "/" + m_sFilter;
	}
	if(m_sFilter.find('?') != std::string::npos)
	{
		m_matcher = &CPathFinder::QuestionMarkFilterMatcher;
	}
	else
	{
		m_matcher = &CPathFinder::StarFilterMatcher;
	}

	SearchRecurse(m_BasePath);
}

unsigned int CMcServ::CPathFinder::Read(ENTRY* pEntry, unsigned int nSize)
{
	assert(m_nIndex <= m_entries.size());
	unsigned int remaining = m_entries.size() - m_nIndex;
	unsigned int readCount = std::min<unsigned int>(remaining, nSize);
	for(unsigned int i = 0; i < readCount; i++)
	{
		pEntry[i] = m_entries[i + m_nIndex];
	}
	m_nIndex += readCount;
	return readCount;
}

void CMcServ::CPathFinder::SearchRecurse(const filesystem::path& Path)
{
	filesystem::directory_iterator itEnd;

	for(filesystem::directory_iterator itElement(Path);
		itElement != itEnd;
		itElement++)
	{
		boost::filesystem::path relativePath(*itElement);
		std::string relativePathString(relativePath.generic_string());

		//"Extract" a more appropriate relative path from the memory card point of view
		relativePathString.erase(0, m_BasePath.string().size());

		//Attempt to match this against the filter
		if((*this.*m_matcher)(relativePathString.c_str()))
		{
			//Fill in the information
			ENTRY entry;

			//strncpy(reinterpret_cast<char*>(pEntry->sName), sRelativePath.c_str(), 0x1F);
			strncpy(reinterpret_cast<char*>(entry.sName), relativePath.filename().string().c_str(), 0x1F);
			entry.sName[0x1F] = 0;

			if(filesystem::is_directory(*itElement))
			{
				entry.nSize			= 0;
				entry.nAttributes	= 0x8427;
			}
			else
			{
				entry.nSize			= static_cast<uint32>(filesystem::file_size(*itElement));
				entry.nAttributes	= 0x8497;
			}

			m_entries.push_back(entry);
		}

		if(filesystem::is_directory(*itElement))
		{
			SearchRecurse(*itElement);
		}
	}
}

//Based on an algorithm found on http://xoomer.alice.it/acantato/dev/wildcard/wildmatch.html
bool CMcServ::CPathFinder::StarFilterMatcher(const char* sPath)
{
	const char* sPattern;
	const char* s;
	const char* p;
	bool nStar;

	sPattern = m_sFilter.c_str();
	nStar = false;

_loopStart:

	for(s = sPath, p = sPattern; *s; s++, p++)
	{
		switch(*p)
		{
		case '*':
			nStar = true;
			sPath = s;
			sPattern = p;
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
	if(!nStar) return false;
	sPath++;
	goto _loopStart;
}

bool CMcServ::CPathFinder::QuestionMarkFilterMatcher(const char* path)
{
	const char* src = m_sFilter.c_str();
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
