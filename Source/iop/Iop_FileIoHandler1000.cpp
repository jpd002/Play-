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
		*ret = m_ioman->Open(args[0], reinterpret_cast<const char*>(&args[1]));
		break;
	case 1:
		assert(retSize == 4);
		*ret = m_ioman->Close(args[0]);
		break;
	case 2:
		assert(retSize == 4);
		*ret = m_ioman->Read(args[0], args[2], reinterpret_cast<void*>(ram + args[1]));
		break;
	case 3:
		assert(retSize == 4);
		*ret = m_ioman->Write(args[0], args[2], reinterpret_cast<const void*>(ram + args[1]));
		break;
	case 4:
		assert(retSize == 4);
		*ret = m_ioman->Seek(args[0], args[1], args[2]);
		break;
	case 9:
		assert(retSize == 4);
		*ret = m_ioman->Dopen(reinterpret_cast<const char*>(&args[0]));
		break;
	case 10:
		assert(retSize == 4);
		*ret = m_ioman->Dclose(args[0]);
		break;
	case 11:
		assert(retSize == 4);
		*ret = m_ioman->Dread(args[0], reinterpret_cast<Iop::CIoman::DIRENTRY*>(ram + args[1]));
		break;
	case 12:
		assert(retSize == 4);
		*ret = m_ioman->GetStat(reinterpret_cast<const char*>(&args[1]), reinterpret_cast<Iop::CIoman::STAT*>(ram + args[0]));
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function (%d) called.\r\n", method);
		break;
	}
}
