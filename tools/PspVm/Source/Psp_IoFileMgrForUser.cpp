#include "Psp_IoFileMgrForUser.h"
#include "Log.h"
#include "alloca_def.h"

#define LOGNAME	("Psp_IoFileMgrForUser")

using namespace Psp;

CIoFileMgrForUser::CIoFileMgrForUser(uint8* ram)
: m_ram(ram)
{

}

CIoFileMgrForUser::~CIoFileMgrForUser()
{

}

std::string CIoFileMgrForUser::GetName() const
{
	return "IoFileMgrForUser";
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

void CIoFileMgrForUser::Invoke(uint32 methodId, CMIPS& context)
{
	switch(methodId)
	{
	case 0x42EC03AC:
		context.m_State.nGPR[CMIPS::V0].nV0 = IoWrite(
			context.m_State.nGPR[CMIPS::A0].nV0,
			context.m_State.nGPR[CMIPS::A1].nV0,
			context.m_State.nGPR[CMIPS::A2].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOGNAME, "Unknown function called 0x%0.8X\r\n", methodId);
		break;
	}
}
