#include <assert.h>
#include <stdio.h>
#include <boost/filesystem/operations.hpp>
#include "Config.h"
#include "IOP_McServ.h"
#include "PS2VM.h"

using namespace IOP;
using namespace Framework;
using namespace boost;
using namespace std;

const char* CMcServ::m_sMcPathPreference[2] =
{
	"ps2.mc0.directory",
	"ps2.mc1.directory",
};

CMcServ::CMcServ()
{
	assert(sizeof(CMD) == 0x414);
	m_nNextHandle = 1;
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

void CMcServ::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x01:
		GetInfo(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x02:
		Open(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x03:
		Close(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x05:
		Read(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0x0D:
		GetDir(pArgs, nArgsSize, pRet, nRetSize);
		break;
	case 0xFE:
		//Get version?
		GetVersionInformation(pArgs, nArgsSize, pRet, nRetSize);
		break;
	default:
		Log("Unknown method invoked (0x%0.8X).\r\n", nMethod);
		break;
	}
}

void CMcServ::SaveState(CStream* pStream)
{

}

void CMcServ::LoadState(CStream* pStream)
{

}

void CMcServ::GetInfo(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nArgsSize >= 0x1C);

	uint32 nPort, nSlot;
	uint32* pRetBuffer;

	bool nWantType;
	bool nWantFreeSpace;
	bool nWantFormatted;
	
	nPort			= ((uint32*)pArgs)[1];
	nSlot			= ((uint32*)pArgs)[2];
	nWantType		= ((uint32*)pArgs)[3] != 0;
	nWantFreeSpace	= ((uint32*)pArgs)[4] != 0;
	nWantFormatted	= ((uint32*)pArgs)[5] != 0;
	pRetBuffer		= (uint32*)&CPS2VM::m_pRAM[((uint32*)pArgs)[7]];

	Log("GetInfo(nPort = %i, nSlot = %i, nWantType = %i, nWantFreeSpace = %i, nWantFormatted = %i, nRetBuffer = 0x%0.8X);\r\n",
		nPort, nSlot, nWantType, nWantFreeSpace, nWantFormatted, ((uint32*)pArgs)[7]);

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

	((uint32*)pRet)[0] = 0;
}

void CMcServ::Open(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	uint32 nHandle;
	CMD* pCmd;
	FILE* pFile;
	const char* sAccess;

	assert(nArgsSize >= 0x414);

	pCmd = reinterpret_cast<CMD*>(pArgs);

	Log("Open(nPort = %i, nSlot = %i, nFlags = %i, sName = %s);\r\n",
		pCmd->nPort, pCmd->nSlot, pCmd->nFlags, pCmd->sName);

	if(pCmd->nPort > 1)
	{
		assert(0);
		((uint32*)pRet)[0] = -1;
		return;
	}

	filesystem::path Path;

	try
	{
		Path = filesystem::path(CConfig::GetInstance()->GetPreferenceString(m_sMcPathPreference[pCmd->nPort]), filesystem::native);
		Path /= pCmd->sName;
	}
	catch(const exception& Exception)
	{
		Log("Error while executing Open: %s\r\n.", Exception.what());
		reinterpret_cast<uint32*>(pRet)[0] = -1;
		return;
	}

	sAccess = NULL;
	switch(pCmd->nFlags)
	{
	case 1:
		sAccess = "rb";
		break;
	}

	if(sAccess == NULL)
	{
		((uint32*)pRet)[0] = -1;
		assert(0);
		return;
	}

	pFile = fopen(Path.string().c_str(), sAccess);
	if(pFile == NULL)
	{
		((uint32*)pRet)[0] = -1;
		return;
	}

	nHandle = GenerateHandle();
	m_Handles[nHandle] = pFile;

	((uint32*)pRet)[0] = nHandle;
}

void CMcServ::Close(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	FILECMD* pCmd;

	pCmd = reinterpret_cast<FILECMD*>(pArgs);

	Log("Close(nHandle = %i);\r\n",
		pCmd->nHandle);

	HandleMap::iterator itHandle;

	itHandle = m_Handles.find(pCmd->nHandle);
	if(itHandle == m_Handles.end())
	{
		((uint32*)pRet)[0] = -1;
		assert(0);
		return;
	}

	fclose(itHandle->second);
	m_Handles.erase(itHandle);

	((uint32*)pRet)[0] = 0;
}

void CMcServ::Read(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	FILECMD* pCmd;
	FILE* pFile;
	void* pDst;

	pCmd = reinterpret_cast<FILECMD*>(pArgs);

	Log("Read(nHandle = %i, nSize = 0x%0.8X, nBufferAddress = 0x%0.8X, nParamAddress = 0x%0.8X);\r\n",
		pCmd->nHandle, pCmd->nSize, pCmd->nBufferAddress, pCmd->nParamAddress);

	HandleMap::iterator itHandle;

	itHandle = m_Handles.find(pCmd->nHandle);
	if(itHandle == m_Handles.end())
	{
		((uint32*)pRet)[0] = -1;
		assert(0);
		return;
	}

	pFile = itHandle->second;
	pDst = &CPS2VM::m_pRAM[pCmd->nBufferAddress];

	//This param buffer is used in the callback after calling this method... No clue what it's for
	((uint32*)&CPS2VM::m_pRAM[pCmd->nParamAddress])[0] = 0;
	((uint32*)&CPS2VM::m_pRAM[pCmd->nParamAddress])[1] = 0;

	((uint32*)pRet)[0] = static_cast<uint32>(fread(pDst, 1, pCmd->nSize, pFile));
}

void CMcServ::GetDir(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	uint32 nRet;

	nRet = 0;

	assert(nArgsSize >= 0x414);

	CMD* pCmd;
	pCmd = reinterpret_cast<CMD*>(pArgs);

	Log("GetDir(nPort = %i, nSlot = %i, nFlags = %i, nMaxEntries = %i, nTableAddress = 0x%0.8X, sName = %s);\r\n",
		pCmd->nPort, pCmd->nSlot, pCmd->nFlags, pCmd->nMaxEntries, pCmd->nTableAddress, pCmd->sName);

	if(pCmd->nPort > 1)
	{
		assert(0);
		((uint32*)pRet)[0] = -1;
		return;
	}

	try
	{
		filesystem::path McPath(CConfig::GetInstance()->GetPreferenceString(m_sMcPathPreference[pCmd->nPort]), filesystem::native);
		McPath = filesystem::complete(McPath);

		if(filesystem::exists(McPath))
		{
			CPathFinder PathFinder(McPath, reinterpret_cast<CPathFinder::ENTRY*>(&CPS2VM::m_pRAM[pCmd->nTableAddress]), pCmd->nMaxEntries, pCmd->sName);
			nRet = PathFinder.Search();
		}
	}
	catch(const exception& Exception)
	{
		Log("Error while executing GetDir: %s\r\n.", Exception.what());
	}

	((uint32*)pRet)[0] = nRet;
}

void CMcServ::GetVersionInformation(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	assert(nArgsSize == 0x30);
	assert(nRetSize == 0x0C);

	((uint32*)pRet)[0] = 0x00000000;
	((uint32*)pRet)[1] = 0x0000020A;		//mcserv version
	((uint32*)pRet)[2] = 0x0000020E;		//mcman version

	Log("Init();\r\n");
}

uint32 CMcServ::GenerateHandle()
{
	return m_nNextHandle++;
}

void CMcServ::Log(const char* sFormat, ...)
{
#ifdef _DEBUG

	if(!CPS2VM::m_Logging.GetIOPLoggingStatus()) return;

	va_list Args;
	printf("IOP_McServ: ");
	va_start(Args, sFormat);
	vprintf(sFormat, Args);
	va_end(Args);

#endif
}

/////////////////////////////////////////////
//CPathFinder Implementation
/////////////////////////////////////////////

CMcServ::CPathFinder::CPathFinder(const filesystem::path& BasePath, ENTRY* pEntry, unsigned int nMax, const char* sFilter)
{
	m_BasePath	= BasePath;
	m_pEntry	= pEntry;
	m_nIndex	= 0;
	m_nMax		= nMax;
	m_sFilter	= sFilter;
}

CMcServ::CPathFinder::~CPathFinder()
{
	
}

unsigned int CMcServ::CPathFinder::Search()
{
	SearchRecurse(m_BasePath);
	return m_nIndex;
}

void CMcServ::CPathFinder::SearchRecurse(const filesystem::path& Path)
{
	filesystem::directory_iterator itEnd;

	for(filesystem::directory_iterator itElement(Path);
		itElement != itEnd;
		itElement++)
	{
		string sRelativePath((*itElement).string());

		//"Extract" a more appropriate relative path from the memory card point of view
		sRelativePath.erase(0, m_BasePath.string().size());

		//Attempt to match this against the filter
		if(MatchesFilter(sRelativePath.c_str()))
		{
			//This fits... fill in the information

			ENTRY* pEntry;
			pEntry = &m_pEntry[m_nIndex];

			//strncpy(reinterpret_cast<char*>(pEntry->sName), sRelativePath.c_str(), 0x1F);
			strncpy(reinterpret_cast<char*>(pEntry->sName), (*itElement).leaf().c_str(), 0x1F);
			pEntry->sName[0x1F] = 0;

			if(filesystem::is_directory(*itElement))
			{
				pEntry->nSize		= 0;
				pEntry->nAttributes	= 0x8427;
			}
			else
			{
				pEntry->nSize		= static_cast<uint32>(filesystem::file_size(*itElement));
				pEntry->nAttributes = 0x8497;
			}

			m_nIndex++;
		}

		if(filesystem::is_directory(*itElement))
		{
			SearchRecurse(*itElement);
		}
	}
}

//Based on an algorithm found on http://xoomer.alice.it/acantato/dev/wildcard/wildmatch.html
bool CMcServ::CPathFinder::MatchesFilter(const char* sPath)
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
