#pragma once

#include "UsbDevice.h"

class CIopBios;

namespace Iop
{
	class CGunCon2UsbDevice : public CUsbDevice
	{
	public:
		enum
		{
			DEVICE_ID = 0xFEEB,
			CONTROL_PIPE_ID = 0x321,
			PIPE_ID = 0x654,
		};
        
        enum : uint32
        {
            GUN_C = 0x0002,
            GUN_B = 0x0004,
            GUN_A = 0x0008,
            GUN_UP = 0x0010,
            GUN_RIGHT = 0x0020,
            GUN_DOWN = 0x0040,
            GUN_LEFT = 0x0080,
            GUN_PROGRESSIVE = 0x0100,
            GUN_TRIGGER = 0x2000,
            GUN_SELECT = 0x4000,
            GUN_START = 0x8000,
            GUN_CALIBRATE = 0x10000,
            GUN_OFFSCREEN = 0x20000
        };

		CGunCon2UsbDevice(CIopBios&, uint8*, int);

		uint16 GetId() const override;
		const char* GetLldName() const override;

		void SaveState(CRegisterState&) const override;
		void LoadState(const CRegisterState&) override;

		void CountTicks(uint32) override;

		void OnLldRegistered() override;
		uint32 ScanStaticDescriptor(uint32, uint32, uint32) override;
		int32 OpenPipe(uint32, uint32) override;
		int32 TransferPipe(uint32, uint32, uint32, uint32, uint32, uint32) override;
        
        void SetParameters(unsigned char*);

		void SetGunState(uint32, int32, int32, bool);

	private:
		CIopBios& m_bios;
		uint8* m_ram = nullptr;

        int32 m_calibration_timer = 0;
        uint32 m_x = 0;
        uint32 m_y = 0;
		uint32 m_buttonState = 0;
		uint32 m_descriptorMemPtr = 0;
		int32 m_nextTransferTicks = 0;
		uint32 m_transferBufferPtr = 0;
		uint32 m_transferSize = 0;
		uint32 m_transferCb = 0;
		uint32 m_transferCbArg = 0;
        int32 m_dx = 0;
        int32 m_dy = 0;
        int m_instance = 0;
        bool m_progressive = 0;
	};
}
