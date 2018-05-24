#include "Iop_FileIoHandler1000.h"
#include "Iop_Ioman.h"
#include "../Log.h"

#define LOG_NAME ("iop_fileio")

using namespace Iop;

CFileIoHandler1000::CFileIoHandler1000(CIoman* ioman)
    : CHandler(ioman)
{
}

void CFileIoHandler1000::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
		assert(retSize == 4);
		*ret = m_ioman->Open(*args, reinterpret_cast<char*>(args) + 4);
		break;
	case 1:
		assert(retSize == 4);
		*ret = m_ioman->Close(*args);
		break;
	case 2:
		assert(retSize == 4);
		*ret = m_ioman->Read(args[0], args[2], reinterpret_cast<void*>(ram + args[1]));
		break;
	case 3:
		assert(retSize == 4);
		*ret = m_ioman->Write(args[0], args[2], reinterpret_cast<void*>(ram + args[1]));
		break;
	case 4:
		assert(retSize == 4);
		*ret = m_ioman->Seek(args[0], args[1], args[2]);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function (%d) called.\r\n", method);
		break;
	}
}
