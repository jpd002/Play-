#pragma once

#include <thread>
#include <future>
#include "filesystem_def.h"
#include "Types.h"
#include "MIPS.h"
#include "MailBox.h"
#include "PadHandler.h"
#include "OpticalMedia.h"
#include "VirtualMachine.h"
#include "ee/Ee_SubSystem.h"
#include "iop/Iop_SubSystem.h"
#include "../tools/PsfPlayer/Source/SoundHandler.h"
#include "FrameDump.h"
#include "FrameLimiter.h"
#include "Profiler.h"

class CPS2VM : public CVirtualMachine
{
public:
	struct CPU_UTILISATION_INFO
	{
		int32 eeTotalTicks = 0;
		int32 eeIdleTicks = 0;

		int32 iopTotalTicks = 0;
		int32 iopIdleTicks = 0;
	};

	typedef std::unique_ptr<Ee::CSubSystem> EeSubSystemPtr;
	typedef std::unique_ptr<Iop::CSubSystem> IopSubSystemPtr;
	typedef std::function<void(const CFrameDump&)> FrameDumpCallback;
	typedef Framework::CSignal<void(const CProfiler::ZoneArray&)> ProfileFrameDoneSignal;

	CPS2VM();
	virtual ~CPS2VM() = default;

	void Initialize();
	void Destroy();

	void StepEe();
	void StepIop();
	void StepVu0();
	void StepVu1();

	void Resume() override;
	void Pause() override;
	void PauseAsync();
	void Reset();

	STATUS GetStatus() const override;

	void DumpEEIntcHandlers();
	void DumpEEDmacHandlers();

	void CreateGSHandler(const CGSHandler::FactoryFunction&);
	CGSHandler* GetGSHandler();
	void DestroyGSHandler();

	void CreatePadHandler(const CPadHandler::FactoryFunction&);
	CPadHandler* GetPadHandler();
	void DestroyPadHandler();

	void CreateSoundHandler(const CSoundHandler::FactoryFunction&);
	CSoundHandler* GetSoundHandler();
	void DestroySoundHandler();
	void ReloadSpuBlockCount();

	void ReloadFrameRateLimit();

	static fs::path GetStateDirectoryPath();
	fs::path GenerateStatePath(unsigned int) const;

	std::future<bool> SaveState(const fs::path&);
	std::future<bool> LoadState(const fs::path&);

	void TriggerFrameDump(const FrameDumpCallback&);

	CPU_UTILISATION_INFO GetCpuUtilisationInfo() const;

#ifdef DEBUGGER_INCLUDED
	fs::path MakeDebugTagsPackagePath(const char*);
	void LoadDebugTags(const char*);
	void SaveDebugTags(const char*);
#endif

	CPadHandler* m_pad;

	EeSubSystemPtr m_ee;
	IopSubSystemPtr m_iop;

	ProfileFrameDoneSignal ProfileFrameDone;

protected:
	virtual void CreateVM();
	void ResumeImpl();

	CMailBox m_mailBox;

private:
	typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;

	void ResetVM();
	void DestroyVM();
	bool SaveVMState(const fs::path&);
	bool LoadVMState(const fs::path&);

	void SaveVmTimingState(Framework::CZipArchiveWriter&);
	void LoadVmTimingState(Framework::CZipArchiveReader&);

	void ReloadExecutable(const char*, const CPS2OS::ArgumentList&);
	void OnCrtModeChange();

	void PauseImpl();
	void DestroyImpl();

	void CreateGsHandlerImpl(const CGSHandler::FactoryFunction&);
	void DestroyGsHandlerImpl();

	void CreatePadHandlerImpl(const CPadHandler::FactoryFunction&);
	void DestroyPadHandlerImpl();

	void CreateSoundHandlerImpl(const CSoundHandler::FactoryFunction&);
	void DestroySoundHandlerImpl();

	void UpdateEe();
	void UpdateIop();
	void UpdateSpu();

	void OnGsNewFrame();

	void CDROM0_SyncPath();
	void CDROM0_Reset();
	void SetIopOpticalMedia(COpticalMedia*);

	void RegisterModulesInPadHandler();

	void EmuThread();

	std::thread m_thread;
	STATUS m_nStatus;
	bool m_nEnd;

	uint32 m_onScreenTicksTotal = 0;
	uint32 m_vblankTicksTotal = 0;
	int m_vblankTicks = 0;
	bool m_inVblank = 0;
	int m_spuUpdateTicks = 0;
	int m_eeExecutionTicks = 0;
	int m_iopExecutionTicks = 0;
	CFrameLimiter m_frameLimiter;

	CPU_UTILISATION_INFO m_cpuUtilisation;

	bool m_singleStepEe;
	bool m_singleStepIop;
	bool m_singleStepVu0;
	bool m_singleStepVu1;

	CFrameDump m_frameDump;
	FrameDumpCallback m_frameDumpCallback;
	std::mutex m_frameDumpCallbackMutex;
	bool m_dumpingFrame = false;

	OpticalMediaPtr m_cdrom0;

	//SPU update parameters
	enum
	{
		DST_SAMPLE_RATE = 44100,
		UPDATE_RATE = 1000, //Number of SPU updates per second (on PS2 time scale)
		SPU_UPDATE_TICKS = PS2::IOP_CLOCK_OVER_FREQ / UPDATE_RATE,
		SAMPLE_COUNT = DST_SAMPLE_RATE / UPDATE_RATE,
		BLOCK_SIZE = SAMPLE_COUNT * 2,
		BLOCK_COUNT = 400,
	};

	int16 m_samples[BLOCK_SIZE * BLOCK_COUNT];
	int m_currentSpuBlock = 0;
	int m_spuBlockCount;
	CSoundHandler* m_soundHandler = nullptr;

	CProfiler::ZoneHandle m_eeProfilerZone = 0;
	CProfiler::ZoneHandle m_iopProfilerZone = 0;
	CProfiler::ZoneHandle m_spuProfilerZone = 0;
	CProfiler::ZoneHandle m_gsSyncProfilerZone = 0;
	CProfiler::ZoneHandle m_otherProfilerZone = 0;

	CPS2OS::RequestLoadExecutableEvent::Connection m_OnRequestLoadExecutableConnection;
	Framework::CSignal<void()>::Connection m_OnCrtModeChangeConnection;
	Framework::CSignal<void(uint32)>::Connection m_OnNewFrameConnection;
};
