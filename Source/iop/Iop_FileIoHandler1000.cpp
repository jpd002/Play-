#include <cstring>
#include "Iop_FileIoHandler1000.h"
#include "Iop_Ioman.h"
#include "Iop_SifManPs2.h"
#include "IopBios.h"
#include "../Log.h"

#define LOG_NAME ("iop_fileio")

#define STATE_XML ("iop_fileio/state1000.xml")
#define STATE_MODULEDATAADDR ("moduleDataAddr")
#define STATE_TRAMPOLINEADDR ("trampolineAddr")

using namespace Iop;

#define MODULEDATA_SIZE 0x100

#define MODULE_VERSION 0x101

#define CUSTOM_EXECREQUEST 0x666
#define CUSTOM_FINISHREQUEST 0x667

#define METHOD_ID_OPEN 0
#define METHOD_ID_CLOSE 1
#define METHOD_ID_READ 2
#define METHOD_ID_SEEK 4

CFileIoHandler1000::CFileIoHandler1000(CIopBios& bios, uint8* iopRam, CIoman* ioman, CSifMan& sifMan)
    : CHandler(ioman)
    , m_bios(bios)
    , m_iopRam(iopRam)
    , m_sifMan(sifMan)
{
}

void CFileIoHandler1000::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_XML));
	m_moduleDataAddr = registerFile.GetRegister32(STATE_MODULEDATAADDR);
	m_bufferAddr = m_moduleDataAddr + offsetof(MODULEDATA, buffer);
	m_trampolineAddr = registerFile.GetRegister32(STATE_TRAMPOLINEADDR);
}

void CFileIoHandler1000::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = new CRegisterStateFile(STATE_XML);
	registerFile->SetRegister32(STATE_MODULEDATAADDR, m_moduleDataAddr);
	registerFile->SetRegister32(STATE_TRAMPOLINEADDR, m_trampolineAddr);
	archive.InsertFile(registerFile);
}

void CFileIoHandler1000::AllocateMemory()
{
	auto sysmem = m_bios.GetSysmem();
	m_moduleDataAddr = sysmem->AllocateMemory(sizeof(MODULEDATA), 0, 0);
	m_bufferAddr = m_moduleDataAddr + offsetof(MODULEDATA, buffer);
	BuildExportTable();
}

void CFileIoHandler1000::ReleaseMemory()
{
	auto sysmem = m_bios.GetSysmem();
	uint32 result = sysmem->FreeMemory(m_moduleDataAddr);
	assert(result == 0);
}

void CFileIoHandler1000::Invoke(CMIPS& context, uint32 method)
{
	switch(method)
	{
	case CUSTOM_EXECREQUEST:
		ExecuteRequest(context);
		break;
	case CUSTOM_FINISHREQUEST:
		FinishRequest(context);
		break;
	default:
		throw std::exception();
		break;
	}
}

bool CFileIoHandler1000::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case METHOD_ID_OPEN:
		LaunchOpenRequest(args, argsSize, ret, retSize, ram);
		return false;
		break;
	case METHOD_ID_CLOSE:
		LaunchCloseRequest(args, argsSize, ret, retSize, ram);
		return false;
		break;
	case METHOD_ID_READ:
		LaunchReadRequest(args, argsSize, ret, retSize, ram);
		return false;
		break;
	case 3:
		assert(retSize == 4);
		*ret = m_ioman->Write(args[0], args[2], reinterpret_cast<const void*>(ram + args[1]));
		break;
	case METHOD_ID_SEEK:
		LaunchSeekRequest(args, argsSize, ret, retSize, ram);
		return false;
		break;
	case 7:
		assert(retSize == 4);
		*ret = m_ioman->Mkdir(reinterpret_cast<const char*>(&args[0]));
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
		*ret = m_ioman->Dread(args[0], reinterpret_cast<Ioman::DIRENTRY*>(ram + args[1]));
		break;
	case 12:
		assert(retSize == 4);
		*ret = m_ioman->GetStat(reinterpret_cast<const char*>(&args[1]), reinterpret_cast<Ioman::STAT*>(ram + args[0]));
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown function (%d) called.\r\n", method);
		break;
	}
	return true;
}

void CFileIoHandler1000::LaunchOpenRequest(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_iopRam + m_moduleDataAddr);
	moduleData->method = METHOD_ID_OPEN;
	moduleData->resultAddr = static_cast<uint32>(reinterpret_cast<uint8*>(ret) - ram);
	strncpy(
	    reinterpret_cast<char*>(moduleData->buffer),
	    reinterpret_cast<const char*>(&args[1]), BUFFER_SIZE);
	m_bios.TriggerCallback(m_trampolineAddr, m_bufferAddr, args[0]);
}

void CFileIoHandler1000::LaunchCloseRequest(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_iopRam + m_moduleDataAddr);
	moduleData->method = METHOD_ID_CLOSE;
	moduleData->resultAddr = static_cast<uint32>(reinterpret_cast<uint8*>(ret) - ram);
	m_bios.TriggerCallback(m_trampolineAddr, args[0]);
}

void CFileIoHandler1000::LaunchReadRequest(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_iopRam + m_moduleDataAddr);
	moduleData->method = METHOD_ID_READ;
	moduleData->resultAddr = static_cast<uint32>(reinterpret_cast<uint8*>(ret) - ram);
	moduleData->eeBufferAddr = args[1];
	moduleData->size = args[2];
	moduleData->bytesProcessed = 0;
	moduleData->handle = args[0];
	m_bios.TriggerCallback(m_trampolineAddr);
}

void CFileIoHandler1000::LaunchSeekRequest(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize == 4);
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_iopRam + m_moduleDataAddr);
	moduleData->resultAddr = static_cast<uint32>(reinterpret_cast<uint8*>(ret) - ram);
	moduleData->method = METHOD_ID_SEEK;
	m_bios.TriggerCallback(m_trampolineAddr, args[0], args[1], args[2]);
}

std::pair<bool, int32> CFileIoHandler1000::FinishReadRequest(MODULEDATA* moduleData, uint8* eeRam, int32 result)
{
	bool done = false;
	if(result < 0)
	{
		//Something failed, abort
		done = true;
	}
	else if(result == 0)
	{
		//We didn't read anything this time, probably got EOF
		result = moduleData->bytesProcessed;
		done = true;
	}
	else
	{
		memcpy(eeRam + moduleData->eeBufferAddr, moduleData->buffer, result);
		moduleData->bytesProcessed += result;
		moduleData->eeBufferAddr += result;
		moduleData->size -= result;
		if(moduleData->size == 0)
		{
			result = moduleData->bytesProcessed;
			done = true;
		}
		else
		{
			done = false;
		}
	}
	return std::make_pair(done, result);
}

void CFileIoHandler1000::ExecuteRequest(CMIPS& context)
{
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_iopRam + m_moduleDataAddr);
	switch(moduleData->method)
	{
	case METHOD_ID_OPEN:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(m_ioman->OpenVirtual(context));
		break;
	case METHOD_ID_CLOSE:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(m_ioman->CloseVirtual(context));
		break;
	case METHOD_ID_READ:
		context.m_State.nGPR[CMIPS::A0].nV0 = moduleData->handle;
		context.m_State.nGPR[CMIPS::A1].nV0 = m_bufferAddr;
		context.m_State.nGPR[CMIPS::A2].nV0 = std::min<uint32>(moduleData->size, BUFFER_SIZE);
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(m_ioman->ReadVirtual(context));
		break;
	case METHOD_ID_SEEK:
		context.m_State.nGPR[CMIPS::V0].nD0 = static_cast<int32>(m_ioman->SeekVirtual(context));
		break;
	default:
		assert(false);
		break;
	}
}

void CFileIoHandler1000::FinishRequest(CMIPS& context)
{
	int32 result = context.m_State.nGPR[CMIPS::A0].nV0;
	auto moduleData = reinterpret_cast<MODULEDATA*>(m_iopRam + m_moduleDataAddr);

	uint8* eeRam = nullptr;
	if(auto sifManPs2 = dynamic_cast<CSifManPs2*>(&m_sifMan))
	{
		eeRam = sifManPs2->GetEeRam();
	}

	bool done = false;
	switch(moduleData->method)
	{
	case METHOD_ID_OPEN:
	case METHOD_ID_SEEK:
	case METHOD_ID_CLOSE:
		done = true;
		break;
	case METHOD_ID_READ:
		std::tie(done, result) = FinishReadRequest(moduleData, eeRam, result);
		break;
	default:
		break;
	}

	if(done)
	{
		*reinterpret_cast<uint32*>(eeRam + moduleData->resultAddr) = result;
		m_sifMan.SendCallReply(CFileIo::SIF_MODULE_ID, nullptr);
		context.m_State.nGPR[CMIPS::V0].nV0 = 0;
	}
	else
	{
		context.m_State.nGPR[CMIPS::V0].nV0 = 1;
	}
}

void CFileIoHandler1000::BuildExportTable()
{
	auto exportTable = reinterpret_cast<uint32*>(m_iopRam + m_moduleDataAddr);
	*(exportTable++) = 0x41E00000;
	*(exportTable++) = 0;
	*(exportTable++) = MODULE_VERSION;
	strcpy(reinterpret_cast<char*>(exportTable), CFileIo::g_moduleId);
	exportTable += (strlen(CFileIo::g_moduleId) + 3) / 4;

	{
		CMIPSAssembler assembler(exportTable);

		uint32 execRequestAddr = (reinterpret_cast<uint8*>(exportTable) - m_iopRam) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_EXECREQUEST);

		uint32 finishRequestAddr = (reinterpret_cast<uint8*>(exportTable) - m_iopRam) + (assembler.GetProgramSize() * 4);
		assembler.JR(CMIPS::RA);
		assembler.ADDIU(CMIPS::R0, CMIPS::R0, CUSTOM_FINISHREQUEST);

		//Assemble trampoline
		{
			static const int16 stackAlloc = 0x10;

			m_trampolineAddr = (reinterpret_cast<uint8*>(exportTable) - m_iopRam) + (assembler.GetProgramSize() * 4);
			auto execRequestLabel = assembler.CreateLabel();

			assembler.ADDIU(CMIPS::SP, CMIPS::SP, -stackAlloc);
			assembler.SW(CMIPS::RA, 0x00, CMIPS::SP);

			assembler.MarkLabel(execRequestLabel);
			assembler.JAL(execRequestAddr);
			assembler.NOP();

			assembler.JAL(finishRequestAddr);
			assembler.ADDIU(CMIPS::A0, CMIPS::V0, CMIPS::R0);

			assembler.BNE(CMIPS::V0, CMIPS::R0, execRequestLabel);
			assembler.NOP();

			assembler.LW(CMIPS::RA, 0x00, CMIPS::SP);
			assembler.JR(CMIPS::RA);
			assembler.ADDIU(CMIPS::SP, CMIPS::SP, stackAlloc);
		}

		auto nextAddress = exportTable + assembler.GetProgramSize();
		auto finalAddress = reinterpret_cast<uint32*>(m_iopRam + m_moduleDataAddr + TRAMPOLINE_SIZE);
		assert(nextAddress <= finalAddress);
	}
}
