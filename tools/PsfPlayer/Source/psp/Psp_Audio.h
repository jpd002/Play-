#pragma once

#include "PspModule.h"
#include "Stream.h"

namespace Psp
{
	class CAudio : public CModule
	{
	public:
		CAudio(uint8*);

		void Invoke(uint32, CMIPS&) override;
		std::string GetName() const override;

		void SetStream(Framework::CStream*);

	private:
		struct CHANNEL
		{
			bool reserved;
			uint32 dataLength;
		};

		enum
		{
			MAX_CHANNEL = 4,
		};

		uint32 ChReserve(uint32, uint32, uint32);
		uint32 SetChannelDataLen(uint32, uint32);
		uint32 OutputPannedBlocking(uint32, uint32, uint32, uint32);

		CHANNEL m_channels[MAX_CHANNEL];
		uint8* m_ram = nullptr;
		Framework::CStream* m_stream = nullptr;
	};

	typedef std::shared_ptr<CAudio> AudioModulePtr;
}
