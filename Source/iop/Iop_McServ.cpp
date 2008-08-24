#include <assert.h>
#include <stdio.h>
#include <boost/filesystem/operations.hpp>
#include <boost/static_assert.hpp>
#include "../AppConfig.h"
#include "../Log.h"
#include "Iop_McServ.h"

using namespace Iop;
using namespace Framework;
using namespace boost;
using namespace std;

#define LOG_NAME ("iop_mcserv")

const char* CMcServ::m_sMcPathPreference[2] =
{
	"ps2.mc0.directory",
	"ps2.mc1.directory",
};

CMcServ::CMcServ(CSIF& sif) :
m_nNextHandle(1)
{
	BOOST_STATIC_ASSERT(sizeof(CMD) == 0x414);
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

string CMcServ::GetId() const
{
    return "mcserv";
}

void CMcServ::Invoke(CMIPS& context, unsigned int functionId)
{
    throw runtime_error("Not implemented.");
}

void CMcServ::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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
}
/*
void CMcServ::SaveState(CStream* pStream)
{

}

void CMcServ::LoadState(CStream* pStream)
{

}
*/
void CMcServ::GetInfo(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 0x1C);

	uint32 nPort			= args[1];
	uint32 nSlot			= args[2];
	bool nWantType		    = args[3] != 0;
	bool nWantFreeSpace	    = args[4] != 0;
	bool nWantFormatted	    = args[5] != 0;
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

	filesystem::path Path;

	try
	{
		Path = filesystem::path(CAppConfig::GetInstance().GetPreferenceString(m_sMcPathPreference[pCmd->nPort]), filesystem::native);
		Path /= pCmd->sName;
	}
	catch(const exception& Exception)
	{
		CLog::GetInstance().Print(LOG_NAME, "Error while executing Open: %s\r\n.", Exception.what());
		ret[0] = -1;
		return;
	}

    if(pCmd->nFlags == 0x40)
    {
        //Directory only?
        uint32 result = -1;
        try
        {
            filesystem::create_directory(Path);
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
	    case 0x01:
            //RDONLY
		    sAccess = "rb";
		    break;
        case 0x02:
            //WRONLY
            sAccess = "r+b";
            break;
        case 0x202:
            //CREATE WRITE
            sAccess = "wb";
            break;
	    }

	    if(sAccess == NULL)
	    {
		    ret[0] = -1;
		    assert(0);
		    return;
	    }

	    FILE* pFile = fopen(Path.string().c_str(), sAccess);
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
		filesystem::path McPath(CAppConfig::GetInstance().GetPreferenceString(m_sMcPathPreference[pCmd->nPort]), filesystem::native);
        McPath /= pCmd->sName;

        if(filesystem::exists(McPath) && filesystem::is_directory(McPath))
		{
            nRet = 0;
		}
	}
	catch(const exception& Exception)
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
		filesystem::path McPath(CAppConfig::GetInstance().GetPreferenceString(m_sMcPathPreference[pCmd->nPort]), filesystem::native);
		McPath = filesystem::complete(McPath);

		if(filesystem::exists(McPath))
		{
			CPathFinder PathFinder(McPath, reinterpret_cast<CPathFinder::ENTRY*>(&ram[pCmd->nTableAddress]), pCmd->nMaxEntries, pCmd->sName);
			nRet = PathFinder.Search();
		}
	}
	catch(const exception& Exception)
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

CMcServ::CPathFinder::CPathFinder(const filesystem::path& BasePath, ENTRY* pEntry, unsigned int nMax, const char* sFilter)
{
	m_BasePath	= BasePath;
	m_pEntry	= pEntry;
	m_nIndex	= 0;
	m_nMax		= nMax;
	m_sFilter	= sFilter;
    if(m_sFilter[0] != '/')
    {
        m_sFilter = "/" + m_sFilter;
    }
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
            if(m_nIndex < m_nMax)
            {
                ENTRY* pEntry = &m_pEntry[m_nIndex];

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
