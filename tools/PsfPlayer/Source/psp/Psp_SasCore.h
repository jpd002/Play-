#pragma once

#include "PspModule.h"
#include "iop/Iop_SpuBase.h"

namespace Psp
{
	class CSasCore : public CModule
	{
	public:
		CSasCore(uint8*);

		void Invoke(uint32, CMIPS&) override;
		std::string GetName() const override;

		void SetSpuInfo(Iop::CSpuSampleCache*, Iop::CSpuBase*, Iop::CSpuBase*, uint8*, uint32);

	private:
		enum REVERBTYPES
		{
			REVERB_OFF = -1,
			REVERB_ROOM = 0,
			REVERB_STUDIOA = 1,
			REVERB_STUDIOB = 2,
			REVERB_STUDIOC = 3,
			REVERB_HALL = 4,
			REVERB_SPACE = 5,
			REVERB_ECHO = 6,
			REVERB_DELAY = 7,
			REVERB_PIPE = 8
		};

		struct REVERBINFO
		{
			uint32 workAreaSize;
			uint16 params[Iop::CSpuBase::REVERB_PARAM_COUNT];
		};

		struct SPUMEMBLOCK
		{
			uint32 address;
			uint32 size;
		};

		typedef std::list<SPUMEMBLOCK> MemBlockList;

		uint32 Init(uint32, uint32, uint32, uint32, uint32);
		uint32 Core(uint32, uint32);
		uint32 SetVoice(uint32, uint32, uint32, uint32, uint32);
		uint32 SetPitch(uint32, uint32, uint32);
		uint32 SetVolume(uint32, uint32, uint32, uint32, uint32, uint32);
		uint32 SetSimpleADSR(uint32, uint32, uint32, uint32);
		uint32 SetKeyOn(uint32, uint32);
		uint32 SetKeyOff(uint32, uint32);
		uint32 GetPauseFlag(uint32);
		uint32 GetEndFlag(uint32);
		uint32 GetAllEnvelope(uint32, uint32);
		uint32 SetEffectType(uint32, uint32);
		uint32 SetEffectParam(uint32, uint32, uint32);
		uint32 SetEffectVolume(uint32, uint32, uint32);
		uint32 SetEffect(uint32, uint32, uint32);

		uint32 AllocMemory(uint32);
		void FreeMemory(uint32);
#ifdef _DEBUG
		void VerifyAllocationMap();
#endif
		void SetupReverb(const REVERBINFO&);

		Iop::CSpuBase::CHANNEL* GetSpuChannel(uint32);

		Iop::CSpuSampleCache* m_spuSampleCache = nullptr;
		Iop::CSpuBase* m_spu[2] = {};
		uint8* m_ram = nullptr;
		uint8* m_spuRam = nullptr;
		uint32 m_spuRamSize = 0;
		uint32 m_grain = 0;

		MemBlockList m_blocks;

		static REVERBINFO g_ReverbStudioC;
		static REVERBINFO g_ReverbHall;
		static REVERBINFO g_ReverbSpace;
	};

	typedef std::shared_ptr<CSasCore> SasCoreModulePtr;
}
