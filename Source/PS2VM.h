#pragma once

#include <thread>
#include <future>
#include "filesystem_def.h"
#include "Types.h"
#include "MIPS.h"
#include "MailBox.h"
#include "PadHandler.h"
#include "GunListener.h"
#include "OpticalMedia.h"
#include "VirtualMachine.h"
#include "ee/Ee_SubSystem.h"
#include "iop/Iop_SubSystem.h"
#include "../tools/PsfPlayer/Source/SoundHandler.h"
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

	typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;
	typedef std::unique_ptr<Ee::CSubSystem> EeSubSystemPtr;
	typedef std::unique_ptr<Iop::CSubSystem> IopSubSystemPtr;
	typedef Framework::CSignal<void()> NewFrameEvent;
	typedef std::function<void(CPS2VM*)> ExecutableReloadedHandler;

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
	void Reset(uint32 = PS2::EE_BASE_RAM_SIZE, uint32 = PS2::IOP_BASE_RAM_SIZE);

	STATUS GetStatus() const override;

	void CreateGSHandler(const CGSHandler::FactoryFunction&);
	CGSHandler* GetGSHandler();
	void DestroyGSHandler();

	void CreatePadHandler(const CPadHandler::FactoryFunction&);
	CPadHandler* GetPadHandler();
	void DestroyPadHandler();

	void CreateSoundHandler(const CSoundHandler::FactoryFunction&);
	CSoundHandler* GetSoundHandler();
	void ReloadSpuBlockCount();
	void DestroySoundHandler();

	void CDROM0_SyncPath();
	void CDROM0_Reset();

	void SetEeFrequencyScale(uint32, uint32);
	void ReloadFrameRateLimit();

	static fs::path GetStateDirectoryPath();
	fs::path GenerateStatePath(unsigned int) const;

	std::future<bool> SaveState(const fs::path&);
	std::future<bool> LoadState(const fs::path&);

	CPU_UTILISATION_INFO GetCpuUtilisationInfo() const;

#ifdef DEBUGGER_INCLUDED
	fs::path MakeDebugTagsPackagePath(const char*);
	void LoadDebugTags(const char*);
	void SaveDebugTags(const char*);
#endif

	void ReportGunPosition(float, float);
	bool HasGunListener() const;
	void SetGunListener(CGunListener*);

	OpticalMediaPtr m_cdrom0;
	CPadHandler* m_pad = nullptr;

	EeSubSystemPtr m_ee;
	IopSubSystemPtr m_iop;

	NewFrameEvent OnNewFrame;

	ExecutableReloadedHandler BeforeExecutableReloaded;
	ExecutableReloadedHandler AfterExecutableReloaded;

protected:
	virtual void CreateVM();
	void ResumeImpl();

	CMailBox m_mailBox;

private:
	void ValidateThreadContext();

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

	void ReloadSpuBlockCountImpl();

	void UpdateEe();
	void UpdateIop();
	void UpdateSpu();

	void SetIopOpticalMedia(COpticalMedia*);

	void RegisterModulesInPadHandler();

	void EmuThread();

	std::thread m_thread;
	STATUS m_nStatus = PAUSED;
	bool m_nEnd = false;

	uint32 m_eeFreqScaleNumerator = 1;
	uint32 m_eeFreqScaleDenominator = 1;
	uint32 m_eeRamSize = PS2::EE_BASE_RAM_SIZE;
	uint32 m_iopRamSize = PS2::IOP_BASE_RAM_SIZE;
	uint32 m_hblankTicksTotal = 0;
	uint32 m_onScreenTicksTotal = 0;
	uint32 m_vblankTicksTotal = 0;
	int m_hblankTicks = 0;
	int m_vblankTicks = 0;
	bool m_inVblank = false;
	int64 m_spuUpdateTicks = 0;
	int64 m_spuUpdateTicksTotal = 0;
	int m_eeExecutionTicks = 0;
	int m_iopExecutionTicks = 0;
	static const int m_eeTickStep = 4800;
	int m_iopTickStep = 0;
	CFrameLimiter m_frameLimiter;

	CPU_UTILISATION_INFO m_cpuUtilisation;

	bool m_singleStepEe = false;
	bool m_singleStepIop = false;
	bool m_singleStepVu0 = false;
	bool m_singleStepVu1 = false;

	//SPU update parameters
	enum
	{
		DST_SAMPLE_RATE = 44100,
		SAMPLES_PER_UPDATE = 45, //44100 / 45 -> 980 SPU updates per second
		SPU_UPDATE_TICKS_PRECISION = 32,
		BLOCK_SIZE = SAMPLES_PER_UPDATE * 2,
		MAX_BLOCK_COUNT = 400,
	};

	int16 m_samples[BLOCK_SIZE * MAX_BLOCK_COUNT];
	int m_currentSpuBlock = 0;
	int m_spuBlockCount = 0;
	CSoundHandler* m_soundHandler = nullptr;

	CGunListener* m_gunListener = nullptr;

	CProfiler::ZoneHandle m_eeProfilerZone = 0;
	CProfiler::ZoneHandle m_iopProfilerZone = 0;
	CProfiler::ZoneHandle m_spuProfilerZone = 0;
	CProfiler::ZoneHandle m_gsSyncProfilerZone = 0;
	CProfiler::ZoneHandle m_otherProfilerZone = 0;

	CPS2OS::RequestLoadExecutableEvent::Connection m_OnRequestLoadExecutableConnection;
	Framework::CSignal<void()>::Connection m_OnCrtModeChangeConnection;
};
