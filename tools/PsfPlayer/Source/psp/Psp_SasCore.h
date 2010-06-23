#ifndef _PSP_SASCORE_H_
#define _PSP_SASCORE_H_

#include "PspModule.h"
#include "iop/Iop_SpuBase.h"

namespace Psp
{
	class CSasCore : public CModule
	{
	public:
									CSasCore(uint8*);
		virtual						~CSasCore();

		void						Invoke(uint32, CMIPS&);
		std::string					GetName() const;

		void						SetSpuInfo(Iop::CSpuBase*, Iop::CSpuBase*, uint8*, uint32);

	private:
		enum REVERBTYPES
		{
			REVERB_OFF		= -1,
			REVERB_ROOM		= 0,
			REVERB_STUDIOA	= 1,
			REVERB_STUDIOB	= 2,
			REVERB_STUDIOC	= 3,
			REVERB_HALL		= 4,
			REVERB_SPACE	= 5,
			REVERB_ECHO		= 6,
			REVERB_DELAY	= 7,
			REVERB_PIPE		= 8
		};

		struct REVERBINFO
		{
			uint32 workAreaSize;
			uint16 params[Iop::CSpuBase::REVERB_PARAM_COUNT];
		};

		struct SPUMEMBLOCK
		{
			uint32		address;
			uint32		size;
		};

		typedef std::list<SPUMEMBLOCK> MemBlockList;

		uint32						Init(uint32, uint32, uint32, uint32, uint32);
		uint32						Core(uint32, uint32);
		uint32						SetVoice(uint32, uint32, uint32, uint32, uint32);
		uint32						SetPitch(uint32, uint32, uint32);
		uint32						SetVolume(uint32, uint32, uint32, uint32, uint32, uint32);
		uint32						SetSimpleADSR(uint32, uint32, uint32, uint32);
		uint32						SetKeyOn(uint32, uint32);
		uint32						SetKeyOff(uint32, uint32);
		uint32						GetPauseFlag(uint32);
		uint32						GetEndFlag(uint32);
		uint32						GetAllEnvelope(uint32, uint32);
		uint32						SetEffectType(uint32, uint32);
		uint32						SetEffectParam(uint32, uint32, uint32);
		uint32						SetEffectVolume(uint32, uint32, uint32);
		uint32						SetEffect(uint32, uint32, uint32);

		uint32						AllocMemory(uint32);
		void						FreeMemory(uint32);
#ifdef _DEBUG
		void						VerifyAllocationMap();
#endif
		void						SetupReverb(const REVERBINFO&);

		Iop::CSpuBase::CHANNEL*		GetSpuChannel(uint32);

		Iop::CSpuBase*				m_spu[2];
		uint8*						m_ram;
		uint8*						m_spuRam;
		uint32						m_spuRamSize;
		uint32						m_grain;

		MemBlockList				m_blocks;
		
		static REVERBINFO			g_ReverbStudioC;
		static REVERBINFO			g_ReverbSpace;
	};

	typedef std::tr1::shared_ptr<CSasCore> SasCoreModulePtr;
}

#endif
