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
	
}

CMcServ::~CMcServ()
{

}

void CMcServ::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x01:
		GetInfo(pArgs, nArgsSize, pRet, nRetSize);
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

void CMcServ::GetDir(void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	struct CMD
	{
		uint32	nPort;
		uint32	nSlot;
		uint32	nFlags;
		uint32	nMaxEntries;
		uint32	nTableAddress;
		char	sName[0x400];
	};

	uint32 nRet;

	nRet = 0;

	assert(sizeof(CMD) == 0x414);
	assert(nArgsSize >= 0x414);

	CMD* pCmd;
	pCmd = reinterpret_cast<CMD*>(pArgs);

	Log("GetDir(nPort = %i, nSlot = %i, nFlags = %i, nMaxEntries = %i, nTableAddress = 0x%0.8X, sName = %s);\r\n",
		pCmd->nPort, pCmd->nSlot, pCmd->nFlags, pCmd->nMaxEntries, pCmd->nTableAddress, pCmd->sName);

	assert(pCmd->nPort < 2);
	pCmd->nPort &= 1;

	filesystem::path McPath(CConfig::GetInstance()->GetPreferenceString(m_sMcPathPreference[pCmd->nPort]), filesystem::native);
	McPath = filesystem::complete(McPath);

	if(filesystem::exists(McPath))
	{
		CPathFinder PathFinder(McPath, reinterpret_cast<CPathFinder::ENTRY*>(&CPS2VM::m_pRAM[pCmd->nTableAddress]), pCmd->nMaxEntries, pCmd->sName);
		nRet = PathFinder.Search();
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
		if(filesystem::is_directory(*itElement))
		{
			//Humm, can this thing return directories too??
			SearchRecurse(*itElement);
		}
		else
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

				pEntry->nSize = static_cast<uint32>(filesystem::file_size(*itElement));

				strncpy(reinterpret_cast<char*>(pEntry->sName), sRelativePath.c_str(), 0x1F);
				pEntry->sName[0x1F] = 0;

				m_nIndex++;
			}
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
