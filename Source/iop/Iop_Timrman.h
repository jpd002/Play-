#pragma once

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CTimrman : public CModule
	{
	public:
		CTimrman(CIopBios&);
		virtual ~CTimrman() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void SaveState(Framework::CZipArchiveWriter&) const override;
		void LoadState(Framework::CZipArchiveReader&) override;

	private:
		int32 AllocHardTimer(uint32, uint32, uint32);
		int ReferHardTimer(uint32, uint32, uint32, uint32);
		int32 FreeHardTimer(uint32);
		void SetTimerMode(CMIPS&, uint32, uint32);
		int GetTimerStatus(CMIPS&, uint32);
		int GetTimerCounter(CMIPS&, uint32);
		void SetTimerCompare(CMIPS&, uint32, uint32);
		int GetHardTimerIntrCode(uint32);
		int32 SetTimerCallback(CMIPS&, uint32, uint32, uint32, uint32);
		int32 SetupHardTimer(CMIPS&, uint32, uint32, uint32, uint32);
		int32 StartHardTimer(CMIPS&, uint32);
		int32 StopHardTimer(CMIPS&, uint32);

		CIopBios& m_bios;
		uint32 m_hardTimerAlloc = 0;
	};
}
