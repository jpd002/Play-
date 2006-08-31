#include <assert.h>
#include <string.h>
#include "IOP_FileIO.h"
#include "PS2VM.h"
#include "Config.h"
#include "StdStream.h"

using namespace IOP;
using namespace Framework;

CFileIO::CFileIO()
{
	CConfig::GetInstance()->RegisterPreferenceBoolean("iop.fileio.stdlogging", false);
	CConfig::GetInstance()->RegisterPreferenceString("ps2.host.directory", "./vfs/host");
	CConfig::GetInstance()->RegisterPreferenceString("ps2.mc0.directory", "./vfs/mc0");
	CConfig::GetInstance()->RegisterPreferenceString("ps2.mc1.directory", "./vfs/mc1");

	m_Device.Insert(new CDirectoryDevice("mc0",		"ps2.mc0.directory"));
	m_Device.Insert(new CDirectoryDevice("host",	"ps2.host.directory"));
	m_Device.Insert(new CCDROM0Device());

	//Insert standard files if requested.
	if(CConfig::GetInstance()->GetPreferenceBoolean("iop.fileio.stdlogging"))
	{
		try
		{
			m_File.Insert(new CStdStream(fopen("ps2_stdout.txt", "ab")), 1);
			m_File.Insert(new CStdStream(fopen("ps2_stderr.txt", "ab")), 2);
		}
		catch(...)
		{
			//Humm, some error occured when opening these files...
		}
	}

	m_nFileID = 3;
}

CFileIO::~CFileIO()
{
	while(m_File.Count() != 0)
	{
		delete m_File.Pull();
	}
	while(m_Device.Count() != 0)
	{
		delete m_Device.Pull();
	}
}

void CFileIO::Invoke(uint32 nMethod, void* pArgs, uint32 nArgsSize, void* pRet, uint32 nRetSize)
{
	switch(nMethod)
	{
	case 0x00000000:
		assert(nRetSize == 4);
		*(uint32*)pRet = Open(*(uint32*)pArgs, ((char*)pArgs) + 4);
		break;
	case 0x00000001:
		assert(nRetSize == 4);
		*(uint32*)pRet = Close(*(uint32*)pArgs);
		break;
	case 0x00000002:
		assert(nRetSize == 4);
		*(uint32*)pRet = Read(((uint32*)pArgs)[0], ((uint32*)pArgs)[2], (void*)(CPS2VM::m_pRAM + ((uint32*)pArgs)[1]));
		break;
	case 0x00000003:
		assert(nRetSize == 4);
		*(uint32*)pRet = Write(((uint32*)pArgs)[0], ((uint32*)pArgs)[2], (void*)(CPS2VM::m_pRAM + ((uint32*)pArgs)[1]));
		break;
	case 0x00000004:
		assert(nRetSize == 4);
		*(uint32*)pRet = Seek(((uint32*)pArgs)[0], ((uint32*)pArgs)[1], ((uint32*)pArgs)[2]);
		break;
	default:
		assert(0);
		break;
	}
}

void CFileIO::SaveState(CStream* pStream)
{

}

void CFileIO::LoadState(CStream* pStream)
{

}

CStream* CFileIO::GetFile(uint32 nMode, const char* sPath)
{
	char sDevice[256];
	char sDevicePath[256];
	CDevice* pDevice;

	if(!SplitPath(sPath, sDevice, sDevicePath))
	{
		return NULL;
	}

	pDevice = FindDevice(sDevice);
	if(pDevice == NULL)
	{
		return NULL;
	}

	return pDevice->OpenFile(nMode, sDevicePath);
}

bool CFileIO::SplitPath(const char* sPath, char* sDevice, char* sDevicePath)
{
	const char* sSplit;
	unsigned int nLenght;
	unsigned int nDevLenght;
	unsigned int nDevPathLenght;

	nLenght = (unsigned int)strlen(sPath);
	sSplit = strchr(sPath, ':');
	if(sSplit == NULL)
	{
		//Invalid path
		return false;
	}

	nDevLenght		= (unsigned int)(sSplit - sPath);
	nDevPathLenght	= (unsigned int)(nLenght - (sSplit - sPath) - 1);

	strncpy(sDevice, sPath, nDevLenght);
	strncpy(sDevicePath, sSplit + 1, nDevPathLenght);

	sDevice[nDevLenght] = '\0';
	sDevicePath[nDevPathLenght] = '\0';

	return true;
}

CFileIO::CDevice* CFileIO::FindDevice(const char* sDeviceName)
{
	CList<CDevice>::ITERATOR itDevice;
	CDevice* pDevice;

	for(itDevice = m_Device.Begin(); itDevice.HasNext(); itDevice++)
	{
		pDevice = (*itDevice);
		if(!strcmp(pDevice->GetName(), sDeviceName))
		{
			return pDevice;
		}
	}

	return NULL;
}

uint32 CFileIO::RegisterFile(CStream* pFile)
{
	uint32 nFD;
	nFD = m_nFileID;
	m_File.Insert(pFile, nFD);
	m_nFileID++;
	return nFD;
}

uint32 CFileIO::Open(uint32 nMode, const char* sPath)
{
/*
	char sDevice[256];
	char sDevicePath[256];
	CDevice* pDevice;
*/
	CStream* pFile;

	Log("Attempting to open file '%s'.\r\n", sPath);
	
	pFile = GetFile(nMode, sPath);
	if(pFile == NULL)
	{
		return 0xFFFFFFFF;
	}

	return RegisterFile(pFile);
/*
	if(!SplitPath(sPath, sDevice, sDevicePath))
	{
		return 0xFFFFFFFF;
	}

	pDevice = FindDevice(sDevice);
	if(pDevice == NULL)
	{
		return 0xFFFFFFFF;
	}

	pFile = pDevice->OpenFile(nMode, sDevicePath);
	if(pFile == NULL)
	{
		return 0xFFFFFFFF;
	}
*/
}

uint32 CFileIO::Close(uint32 nFile)
{
	CStream* pFile;

	pFile = m_File.Remove(nFile);
	if(pFile == NULL)
	{
		return 0xFFFFFFFF;
	}

	delete pFile;

	return 0;
}

uint32 CFileIO::Read(uint32 nFile, uint32 nSize, void* pBuffer)
{
	CStream* pFile;
	
	pFile = m_File.Find(nFile);
	if(pFile == NULL)
	{
		Log("Reading from an unopened file (fd = %i).\r\n", nFile);
		return 0xFFFFFFFF;
	}

	if(pFile->IsEOF())
	{
		return 0;
	}

	return (uint32)pFile->Read(pBuffer, nSize);
}

uint32 CFileIO::Write(uint32 nFile, uint32 nSize, void* pBuffer)
{
	CStream* pFile;
	uint32 nAmount;

	pFile = m_File.Find(nFile);
	if(pFile == NULL)
	{
		if(nFile > 2)
		{
			Log("Writing to an unopened file (fd = %i).\r\n", nFile);
		}
		return 0xFFFFFFFF;
	}

	nAmount = (uint32)pFile->Write(pBuffer, nSize);
	pFile->Flush();
	return nAmount;
}

uint32 CFileIO::Seek(uint32 nFile, uint32 nOffset, uint32 nWhence)
{
	CStream* pFile;
	STREAM_SEEK_DIRECTION nPosition;

	pFile = m_File.Find(nFile);
	if(pFile == NULL)
	{
		Log("Seeking in an unopened file (fd = %i).\r\n", nFile);
		return 0xFFFFFFFF;
	}

	switch(nWhence)
	{
	case 0:
		nPosition = Framework::STREAM_SEEK_SET;
		break;
	case 1:
		nPosition = Framework::STREAM_SEEK_CUR;
		break;
	case 2:
		nPosition = Framework::STREAM_SEEK_END;
		break;
	}

	pFile->Seek(nOffset, nPosition);
	return (uint32)pFile->Tell();
}

void CFileIO::Log(const char* sFormat, ...)
{
#ifdef _DEBUG

	if(!CPS2VM::m_Logging.GetIOPLoggingStatus()) return;

	va_list Args;
	printf("IOP_FileIO: ");
	va_start(Args, sFormat);
	vprintf(sFormat, Args);
	va_end(Args);

#endif
}

/////////////////////////////////////////////
//Devices Implementation
/////////////////////////////////////////////

CFileIO::CDevice::CDevice(const char* sName)
{
	m_sName = sName;
}

const char* CFileIO::CDevice::GetName()
{
	return m_sName;
}

CFileIO::CDirectoryDevice::CDirectoryDevice(const char* sName, const char* sBasePathPreference) :
CDevice(sName)
{
	m_sBasePathPreference = sBasePathPreference;
}

CStream* CFileIO::CDirectoryDevice::OpenFile(uint32 nMode, const char* sDevicePath)
{
	FILE* pStream;
	char sPath[256];
	const char* sMode;
	const char* sBasePath;

	sBasePath = CConfig::GetInstance()->GetPreferenceString(m_sBasePathPreference);

	strcpy(sPath, sBasePath);
	if(sDevicePath[0] != '/')
	{
		strcat(sPath, "/");
	}
	strcat(sPath, sDevicePath);

	switch(nMode)
	{
	case 0:
	case O_RDONLY:
		sMode = "rb";
		break;
	case (O_RDWR | O_CREAT):
		sMode = "w+";
		break;
	default:
		assert(0);
		break;
	}


	pStream = fopen(sPath, sMode);
	if(pStream == NULL) return NULL;

	return new CStdStream(pStream);
}

CFileIO::CCDROM0Device::CCDROM0Device() :
CFileIO::CDevice("cdrom0")
{

}

CFileIO::CCDROM0Device::~CCDROM0Device()
{

}

CStream* CFileIO::CCDROM0Device::OpenFile(uint32 nMode, const char* sDevicePath)
{
	CISO9660* pISO;

	if(nMode != O_RDONLY) return NULL;

	pISO = CPS2VM::m_pCDROM0;
	if(pISO == NULL) return NULL;

	return pISO->Open(sDevicePath);
}
