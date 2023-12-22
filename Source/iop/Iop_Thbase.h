#pragma once

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CThbase : public CModule
	{
	public:
		CThbase(CIopBios&, uint8*);
		virtual ~CThbase() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		struct THREAD
		{
			uint32 attributes;
			uint32 options;
			uint32 threadProc;
			uint32 stackSize;
			uint32 priority;
		};

		uint32 CreateThread(const THREAD*);
		uint32 DeleteThread(uint32);
		int32 StartThread(uint32, uint32);
		int32 StartThreadArgs(uint32, uint32, uint32);
		uint32 ExitThread();
		uint32 TerminateThread(uint32);
		uint32 ChangeThreadPriority(uint32, uint32);
		int32 RotateThreadReadyQueue(uint32);
		int32 ReleaseWaitThread(uint32);
		int32 iReleaseWaitThread(uint32);
		uint32 GetThreadId();
		uint32 ReferThreadStatus(uint32, uint32);
		uint32 iReferThreadStatus(uint32, uint32);
		uint32 SleepThread();
		uint32 WakeupThread(uint32);
		uint32 iWakeupThread(uint32);
		int32 CancelWakeupThread(uint32);
		int32 iCancelWakeupThread(uint32);
		uint32 DelayThread(uint32);
		uint32 GetSystemTime(uint32);
		uint32 GetSystemTimeLow();
		uint32 SetAlarm(uint32, uint32, uint32);
		uint32 CancelAlarm(uint32, uint32);
		uint32 iCancelAlarm(uint32, uint32);
		void USecToSysClock(uint32, uint32);
		void SysClockToUSec(uint32, uint32, uint32);
		uint32 GetCurrentThreadPriority();
		int32 GetThreadmanIdList(uint32, uint32, uint32, uint32);

		uint8* m_ram;
		CIopBios& m_bios;
	};
}
