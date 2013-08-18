#ifndef _PSP_AUDIO_H_
#define _PSP_AUDIO_H_

#include "PspModule.h"
#include "Stream.h"

namespace Psp
{
	class CAudio : public CModule
	{
	public:
								CAudio(uint8*);
		virtual					~CAudio();

		void					Invoke(uint32, CMIPS&);
		std::string				GetName() const;

		void					SetStream(Framework::CStream*);

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

		uint32					ChReserve(uint32, uint32, uint32);
		uint32					SetChannelDataLen(uint32, uint32);
		uint32					OutputPannedBlocking(uint32, uint32, uint32, uint32);

		CHANNEL					m_channels[MAX_CHANNEL];
		uint8*					m_ram;
		Framework::CStream*		m_stream;
	};

	typedef std::shared_ptr<CAudio> AudioModulePtr;
}

#endif
