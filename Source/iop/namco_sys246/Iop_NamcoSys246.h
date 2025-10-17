#pragma once

#include <array>
#include <queue>
#include "../Iop_Module.h"
#include "../Iop_SifMan.h"
#include "../../SifModuleAdapter.h"
#include "../../PadInterface.h"
#include "../../ScreenPositionListener.h"
#include "filesystem_def.h"

namespace Iop
{
	class CSifCmd;

	namespace Namco
	{
		class CAcRam;

		class CSys246 : public CModule, public CPadInterface, public CScreenPositionListener
		{
		public:
			enum class JVS_MODE
			{
				DEFAULT,
				LIGHTGUN,
				DRUM,
				DRIVE,
				TOUCH,
			};

			CSys246(CSifMan&, CSifCmd&, Namco::CAcRam&, const std::string&);
			virtual ~CSys246() = default;

			std::string GetId() const override;
			std::string GetFunctionName(unsigned int) const override;
			void Invoke(CMIPS&, unsigned int) override;

			void SaveState(Framework::CZipArchiveWriter&) const override;
			void LoadState(Framework::CZipArchiveReader&) override;

			void SetJvsMode(JVS_MODE);
			void SetButton(unsigned int, PS2::CControllerInfo::BUTTON);
			void SetScreenPosXform(const std::array<float, 4>&);

			//CPadInterface
			void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) override;
			void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override;
			void GetVibration(unsigned int, uint8& largeMotor, uint8& smallMotor) override{};

			//CScreenPositionListener
			void SetScreenPosition(float, float) override;
			void ReleaseScreenPosition() override;

			// Recoil event callback
			static std::function<void(int)> m_outputCallbackFunction;
			static void SetOutputCallback(std::function<void(int)> callback);

		private:
			enum MODULE_ID
			{
				MODULE_ID_1 = 0x76500001,
				MODULE_ID_3 = 0x76500003,
				MODULE_ID_4 = 0x76500004,
				MODULE_ID_SERIAL = 0x00830120,
			};

			enum COMMAND_ID
			{
				COMMAND_ID_ACFLASH = 3,
			};

			enum
			{
				SERIAL_SETUP = 1,
				SERIAL_2 = 2,
				SERIAL_READ = 3,
				SERIAL_WRITE = 4,
				SERIAL_5 = 5,
				SERIAL_WRITE_READ = 10,
				SERIAL_12 = 12,
				SERIAL_13 = 13,
				SERIAL_14 = 14,
				SERIAL_15 = 15,
			};

			enum
			{
				BACKUP_RAM_SIZE = 0x10000,
			};

			enum
			{
				JVS_PLAYER_COUNT = 2,
			};

			enum JVS_DRUM_CHANNELS
			{
				JVS_DRUM_CHANNEL_1P_DL,
				JVS_DRUM_CHANNEL_2P_KL,
				JVS_DRUM_CHANNEL_2P_DL,
				JVS_DRUM_CHANNEL_1P_DR,
				JVS_DRUM_CHANNEL_1P_KR,
				JVS_DRUM_CHANNEL_1P_KL,
				JVS_DRUM_CHANNEL_2P_KR,
				JVS_DRUM_CHANNEL_2P_DR,
				JVS_DRUM_CHANNEL_MAX,
			};

			enum JVS_WHEEL_CHANNELS
			{
				JVS_WHEEL_CHANNEL_WHEEL,
				JVS_WHEEL_CHANNEL_GAZ,
				JVS_WHEEL_CHANNEL_BRAKE,
				JVS_WHEEL_CHANNEL_MAX,
			};

			bool Invoke001(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke003(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke004(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool InvokeSerial(uint32, uint32*, uint32, uint32*, uint32, uint8*);

			void ProcessAcFlashCommand(const SIFCMDHEADER*, CSifMan&);

			void ProcessJvsPacket(const uint8*, uint8*);
			void ProcessBgStrPacket(const uint8*, uint8*, int);

			static fs::path GetArcadeSavePath();
			void ProcessMemRequest(uint8*, uint32);
			void ReadBackupRam(uint32, uint8*, uint32);
			void WriteBackupRam(uint32, const uint8*, uint32);

			Namco::CAcRam& m_acRam;

			CSifModuleAdapter m_module001;
			CSifModuleAdapter m_module003;
			CSifModuleAdapter m_module004;
			CSifModuleAdapter m_moduleSerial;

			std::string m_gameId;
			uint32 m_recvAddr = 0;
			uint32 m_sendAddr = 0;

			JVS_MODE m_jvsMode = JVS_MODE::DEFAULT;
			std::array<float, 4> m_screenPosXform = {65535, 0, 65535, 0};

			std::array<uint16, PS2::CControllerInfo::MAX_BUTTONS> m_jvsButtonBits = {};
			uint16 m_jvsButtonState[JVS_PLAYER_COUNT] = {};
			uint16 m_jvsSystemButtonState = 0;
			uint16 m_jvsScreenPosX = 0x7FFF;
			uint16 m_jvsScreenPosY = 0x7FFF;
			uint16 m_jvsDrumChannels[JVS_DRUM_CHANNEL_MAX] = {};
			uint16 m_jvsWheelChannels[JVS_WHEEL_CHANNEL_MAX] = {};
			uint16 m_jvsWheel = 0x0;
			uint16 m_jvsGaz = 0x0;
			uint16 m_jvsBrake = 0x0;
			uint16 m_coin1 = 0;
			uint16 m_coin2 = 0;
			uint8 m_testButtonState = 0;
			uint8 m_counter = 0;

			std::queue<uint8> m_serialQueue;
			bool m_bgStrReportWheelPos = false;

			// track last recoil state to limit events
			uint8 m_p1RecoilLast = 0;
		};
	}
}
