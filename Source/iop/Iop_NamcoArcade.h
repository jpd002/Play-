#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "../SifModuleAdapter.h"
#include "../PadListener.h"
#include "../GunListener.h"
#include "filesystem_def.h"

namespace Iop
{
	namespace Namco
	{
		class CAcRam;
	}

	class CNamcoArcade : public CModule, public CPadListener, public CGunListener
	{
	public:
		CNamcoArcade(CSifMan&, Namco::CAcRam&, const std::string&);
		virtual ~CNamcoArcade() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void SaveState(Framework::CZipArchiveWriter&) const override;
		void LoadState(Framework::CZipArchiveReader&) override;

		//CPadListener
		void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) override;
		void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override;

		//CGunListener
		void SetGunPosition(float, float) override;
		
	private:
		enum MODULE_ID
		{
			MODULE_ID_1 = 0x76500001,
			MODULE_ID_3 = 0x76500003,
			MODULE_ID_4 = 0x76500004,
		};

		enum
		{
			BACKUP_RAM_SIZE = 0x10000,
		};

		bool Invoke001(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool Invoke003(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool Invoke004(uint32, uint32*, uint32, uint32*, uint32, uint8*);

		static fs::path GetArcadeSavePath();
		void ReadBackupRam(uint32, uint8*, uint32);
		void WriteBackupRam(uint32, const uint8*, uint32);

		Namco::CAcRam& m_acRam;

		CSifModuleAdapter m_module001;
		CSifModuleAdapter m_module003;
		CSifModuleAdapter m_module004;

		std::string m_gameId;
		uint32 m_recvAddr = 0;
		uint32 m_sendAddr = 0;
	};
}
