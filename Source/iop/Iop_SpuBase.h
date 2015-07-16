#pragma once

#include "Types.h"
#include "BasicUnion.h"
#include "Convertible.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
	class CSpuBase
	{
	public:
		struct ADSR_LEVEL : public convertible<uint16>
		{
			unsigned int	sustainLevel	: 4;
			unsigned int	decayRate		: 4;
			unsigned int	attackRate		: 7;
			unsigned int	attackMode		: 1;
		};
		static_assert(sizeof(ADSR_LEVEL) >= sizeof(uint16), "Size of ADSR_LEVEL struct must be at least 2 bytes.");

		struct ADSR_RATE : public convertible<uint16>
		{
			unsigned int	releaseRate			: 5;
			unsigned int	releaseMode			: 1;
			unsigned int	sustainRate			: 7;
			unsigned int	reserved0			: 1;
			unsigned int	sustainDirection	: 1;
			unsigned int	sustainMode			: 1;
		};
		static_assert(sizeof(ADSR_RATE) >= sizeof(uint16), "Size of ADSR_RATE struct must be at least 2 bytes.");

		struct CHANNEL_VOLUME : public convertible<uint16>
		{
			union
			{
				struct
				{
					unsigned int	unused0		: 15;
					unsigned int	mode		: 1;
				} mode;
				struct
				{
					unsigned int	volume		: 14;
					unsigned int	phase		: 1;
					unsigned int	mode		: 1;
				} volume;
				struct
				{
					unsigned int	volume		: 7;
					unsigned int	unused0		: 5;
					unsigned int	phase		: 1;
					unsigned int	decrease	: 1;
					unsigned int	slope		: 1;
					unsigned int	mode		: 1;
				} sweep;
			};
		};
		static_assert(sizeof(CHANNEL_VOLUME) >= sizeof(uint16), "Size of CHANNEL_VOLUME struct must be at least 2 bytes.");

		enum
		{
			MAX_CHANNEL = 24
		};

		enum
		{
			REVERB_PARAM_COUNT = 32
		};

		enum CONTROL
		{
			CONTROL_REVERB		= 0x80,
			CONTROL_IRQ			= 0x40,

			CONTROL_DMA			= 0x30,
			CONTROL_DMA_IO		= 0x10,
			CONTROL_DMA_WRITE	= 0x20,
			CONTROL_DMA_READ	= 0x30,
		};

		enum TRANSFER_MODE
		{
			TRANSFER_MODE_VOICE			= 0,
			TRANSFER_MODE_BLOCK_CORE0IN	= 1,
			TRANSFER_MODE_BLOCK_CORE1IN	= 2,
			TRANSFER_MODE_BLOCK_READ	= 4
		};

		enum
		{
			FB_SRC_A = 0,
			FB_SRC_B,
			IIR_ALPHA,
			ACC_COEF_A,
			ACC_COEF_B,
			ACC_COEF_C,
			ACC_COEF_D,
			IIR_COEF,
			FB_ALPHA,
			FB_X,
			IIR_DEST_A0,
			IIR_DEST_A1,
			ACC_SRC_A0,
			ACC_SRC_A1,
			ACC_SRC_B0,
			ACC_SRC_B1,
			IIR_SRC_A0,
			IIR_SRC_A1,
			IIR_DEST_B0,
			IIR_DEST_B1,
			ACC_SRC_C0,
			ACC_SRC_C1,
			ACC_SRC_D0,
			ACC_SRC_D1,
			IIR_SRC_B1,
			IIR_SRC_B0,
			MIX_DEST_A0,
			MIX_DEST_A1,
			MIX_DEST_B0,
			MIX_DEST_B1,
			IN_COEF_L,
			IN_COEF_R,
			REVERB_REG_COUNT,
		};

		enum CHANNEL_STATUS
		{
			STOPPED = 0,
			KEY_ON = 1,
			ATTACK,
			DECAY,
			SUSTAIN,
			RELEASE,
		};

		struct CHANNEL
		{
			CHANNEL_VOLUME	volumeLeft;
			CHANNEL_VOLUME	volumeRight;
			int32			volumeLeftAbs;
			int32			volumeRightAbs;
			uint16			pitch;
			uint32			address;
			ADSR_LEVEL		adsrLevel;
			ADSR_RATE		adsrRate;
			uint32			adsrVolume;
			uint32			repeat;
			uint16			status;
			uint32			current;
		};


						CSpuBase(uint8*, uint32, unsigned int);
		virtual			~CSpuBase();

		void			Reset();

		void			LoadState(Framework::CZipArchiveReader&);
		void			SaveState(Framework::CZipArchiveWriter&);

		bool			IsEnabled() const;

		void			SetVolumeAdjust(float);
		void			SetReverbEnabled(bool);

		void			SetBaseSamplingRate(uint32);

		bool			GetIrqPending() const;

		uint32			GetIrqAddress() const;
		void			SetIrqAddress(uint32);

		uint16			GetTransferMode() const;
		void			SetTransferMode(uint16);

		uint32			GetTransferAddress() const;
		void			SetTransferAddress(uint32);

		uint16			GetControl() const;
		void			SetControl(uint16);

		uint32			GetReverbParam(unsigned int) const;
		void			SetReverbParam(unsigned int, uint32);

		uint32			GetReverbWorkAddressStart() const;
		void			SetReverbWorkAddressStart(uint32);

		uint32			GetReverbWorkAddressEnd() const;
		void			SetReverbWorkAddressEnd(uint32);

		void			SetReverbCurrentAddress(uint32);

		UNION32_16		GetChannelOn() const;
		void			SetChannelOn(uint16, uint16);
		void			SetChannelOnLo(uint16);
		void			SetChannelOnHi(uint16);

		UNION32_16		GetChannelReverb() const;
		void			SetChannelReverbLo(uint16);
		void			SetChannelReverbHi(uint16);

		CHANNEL&		GetChannel(unsigned int);

		void			SendKeyOn(uint32);
		void			SendKeyOff(uint32);

		UNION32_16		GetEndFlags() const;
		void			ClearEndFlags();

		void			WriteWord(uint16);

		uint32			ReceiveDma(uint8*, uint32, uint32);

		void			Render(int16*, unsigned int, unsigned int);

		static bool		g_reverbParamIsAddress[REVERB_PARAM_COUNT];

	private:
		class CSampleReader
		{
		public:
							CSampleReader();
			virtual			~CSampleReader();

			void			Reset();
			void			SetMemory(uint8*, uint32);

			void			SetParams(uint32, uint32);
			void			SetPitch(uint32, uint16);
			void			GetSamples(int16*, unsigned int, unsigned int);
			uint32			GetRepeat() const;
			void			SetRepeat(uint32);
			uint32			GetCurrent() const;
			void			SetIrqAddress(uint32);
			bool			IsDone() const;
			bool			GetEndFlag() const;
			void			ClearEndFlag();
			bool			GetIrqPending() const;
			void			ClearIrqPending();

			bool			DidChangeRepeat() const;
			void			ClearDidChangeRepeat();

		private:
			enum
			{
				BUFFER_SAMPLES = 28,
			};

			void			UnpackSamples(int16*);
			void			AdvanceBuffer();
			int16			GetSample(unsigned int);

			uint8*			m_ram = nullptr;
			uint32			m_ramSize = 0;

			uint32			m_srcSampleIdx;
			unsigned int	m_srcSamplingRate;
			uint32			m_nextSampleAddr = 0;
			uint32			m_repeatAddr = 0;
			uint32			m_irqAddr = 0;
			int16			m_buffer[BUFFER_SAMPLES * 2];
			uint16			m_pitch;
			int32			m_s1;
			int32			m_s2;
			bool			m_done;
			bool			m_nextValid;
			bool			m_endFlag;
			bool			m_irqPending = false;
			bool			m_didChangeRepeat;
		};

		enum
		{
			MAX_ADSR_VOLUME = 0x7FFFFFFF,
		};

		void				UpdateAdsr(CHANNEL&);
		uint32				GetAdsrDelta(unsigned int) const;
		float				GetReverbSample(uint32) const;
		void				SetReverbSample(uint32, float);
		uint32				GetReverbOffset(unsigned int) const;
		float				GetReverbCoef(unsigned int) const;

		static void			MixSamples(int32, int32, int16*);
		int32				ComputeChannelVolume(const CHANNEL_VOLUME&, int32);

		static const uint32	g_linearIncreaseSweepDeltas[0x80];
		static const uint32	g_linearDecreaseSweepDeltas[0x80];

		uint8*				m_ram;
		uint32				m_ramSize;
		unsigned int		m_spuNumber;
		uint32				m_baseSamplingRate;
		uint32				m_irqAddr = 0;
		bool				m_irqPending = false;
		uint16				m_transferMode;
		uint32				m_transferAddr;
		UNION32_16			m_channelOn;
		UNION32_16			m_channelReverb;
		uint32				m_reverbWorkAddrStart;
		uint32				m_reverbWorkAddrEnd;
		uint32				m_reverbCurrAddr;
		uint16				m_ctrl;
		int					m_reverbTicks;
		uint32				m_reverb[REVERB_REG_COUNT];
		CHANNEL				m_channel[MAX_CHANNEL];
		CSampleReader		m_reader[MAX_CHANNEL];
		uint32				m_adsrLogTable[160];
		bool				m_reverbEnabled;
		float				m_volumeAdjust;
	};
}
