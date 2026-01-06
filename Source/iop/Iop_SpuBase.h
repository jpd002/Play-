#pragma once

#include <map>
#include "Types.h"
#include "BasicUnion.h"
#include "Convertible.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CRegisterState;

namespace Iop
{
	class CSpuSampleCache
	{
	public:
		static constexpr int BUFFER_SAMPLES = 28;

		struct KEY
		{
			uint32 address;
			int32 s1;
			int32 s2;
		};

		struct ITEM
		{
			int16 samples[BUFFER_SAMPLES];
			int32 inS1;
			int32 inS2;
			int32 outS1;
			int32 outS2;
		};

		const ITEM* GetItem(const KEY&) const;
		ITEM& RegisterItem(const KEY&);
		void Clear();
		void ClearRange(uint32 address, uint32 size);

	private:
		std::multimap<uint32, ITEM> m_cache;
	};

	class CSpuIrqWatcher
	{
	public:
		void Reset();

		void LoadState(Framework::CZipArchiveReader&);
		void SaveState(Framework::CZipArchiveWriter&);

		void SetIrqAddress(int core, uint32 address);
		void CheckIrq(uint32 address);
		void ClearIrqPending(int);
		bool HasPendingIrq(int) const;

	private:
		static constexpr int MAX_CORES = 2;

		uint32 m_irqAddr[MAX_CORES] = {};
		bool m_irqPending[MAX_CORES] = {};
	};

	class CSpuBase
	{
	public:
		struct ADSR_LEVEL : public convertible<uint16>
		{
			unsigned int sustainLevel : 4;
			unsigned int decayRate : 4;
			unsigned int attackRate : 7;
			unsigned int attackMode : 1;
		};
		static_assert(sizeof(ADSR_LEVEL) >= sizeof(uint16), "Size of ADSR_LEVEL struct must be at least 2 bytes.");

		struct ADSR_RATE : public convertible<uint16>
		{
			unsigned int releaseRate : 5;
			unsigned int releaseMode : 1;
			unsigned int sustainRate : 7;
			unsigned int reserved0 : 1;
			unsigned int sustainDirection : 1;
			unsigned int sustainMode : 1;
		};
		static_assert(sizeof(ADSR_RATE) >= sizeof(uint16), "Size of ADSR_RATE struct must be at least 2 bytes.");

		struct CHANNEL_VOLUME : public convertible<uint16>
		{
			union
			{
				struct
				{
					unsigned int unused0 : 15;
					unsigned int mode : 1;
				} mode;
				struct
				{
					unsigned int volume : 14;
					unsigned int phase : 1;
					unsigned int mode : 1;
				} volume;
				struct
				{
					unsigned int volume : 7;
					unsigned int unused0 : 5;
					unsigned int phase : 1;
					unsigned int decrease : 1;
					unsigned int slope : 1;
					unsigned int mode : 1;
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
			CONTROL_REVERB = 0x80,
			CONTROL_IRQ = 0x40,

			CONTROL_DMA = 0x30,
			CONTROL_DMA_STOP = 0x00,
			CONTROL_DMA_IO = 0x10,
			CONTROL_DMA_WRITE = 0x20,
			CONTROL_DMA_READ = 0x30,
		};

		enum TRANSFER_MODE
		{
			TRANSFER_MODE_VOICE = 0,
			TRANSFER_MODE_BLOCK_CORE0IN = 1,
			TRANSFER_MODE_BLOCK_CORE1IN = 2,
			TRANSFER_MODE_BLOCK_READ = 4
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
			IIR_SRC_B0,
			IIR_SRC_B1,
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
			CHANNEL_VOLUME volumeLeft;
			CHANNEL_VOLUME volumeRight;
			int32 volumeLeftAbs;
			int32 volumeRightAbs;
			uint16 pitch;
			uint32 address;
			ADSR_LEVEL adsrLevel;
			ADSR_RATE adsrRate;
			uint32 adsrVolume;
			uint32 repeat;
			bool repeatSet;
			uint32 status;
			uint32 current;
		};

		CSpuBase(uint8*, uint32, CSpuSampleCache*, CSpuIrqWatcher*, unsigned int);
		virtual ~CSpuBase() = default;

		void Reset();

		void LoadState(Framework::CZipArchiveReader&);
		void SaveState(Framework::CZipArchiveWriter&);

		bool IsEnabled() const;

		void SetVolumeAdjust(float);
		void SetReverbEnabled(bool);

		void SetBaseSamplingRate(uint32);
		void SetInputBypass(bool);
		void SetDestinationSamplingRate(uint32);

		bool GetIrqPending() const;
		void ClearIrqPending();

		uint32 GetIrqAddress() const;
		void SetIrqAddress(uint32);

		uint16 GetTransferMode() const;
		void SetTransferMode(uint16);

		uint32 GetTransferAddress() const;
		void SetTransferAddress(uint32);

		uint16 GetControl() const;
		void SetControl(uint16);

		uint32 GetReverbParam(unsigned int) const;
		void SetReverbParam(unsigned int, uint32);

		uint32 GetReverbWorkAddressStart() const;
		void SetReverbWorkAddressStart(uint32);

		uint32 GetReverbWorkAddressEnd() const;
		void SetReverbWorkAddressEnd(uint32);

		void SetReverbCurrentAddress(uint32);

		UNION32_16 GetChannelOn() const;
		void SetChannelOn(uint16, uint16);
		void SetChannelOnLo(uint16);
		void SetChannelOnHi(uint16);

		UNION32_16 GetChannelReverb() const;
		void SetChannelReverbLo(uint16);
		void SetChannelReverbHi(uint16);

		CHANNEL& GetChannel(unsigned int);
		void OnChannelPitchChanged(unsigned int);

		void SendKeyOn(uint32);
		void SendKeyOff(uint32);

		UNION32_16 GetEndFlags() const;
		void ClearEndFlags();

		void WriteWord(uint16);

		uint32 ReceiveDma(uint8*, uint32, uint32, uint32);

		void Render(int16*, unsigned int);

		static bool g_reverbParamIsAddress[REVERB_PARAM_COUNT];

		uint32 m_inputVolL = 0x7FFF;
		uint32 m_inputVolR = 0x7FFF;

		int32 m_extInputVolL = 0x7FFF;
		int32 m_extInputVolR = 0x7FFF;

		static void MixSamples(int32, int32, int16*);

	private:
		enum
		{
			CORE0_SIN_LEFT = 0x000,
			CORE0_SIN_RIGHT = 0x200,
			CORE1_SIN_LEFT = 0x800,
			CORE1_SIN_RIGHT = 0xA00,
			CORE0_OUTPUT_SIZE = 0x200,
			SOUND_INPUT_DATA_CORE0_BASE = 0x2000,
			SOUND_INPUT_DATA_CORE1_BASE = 0x2400,
			SOUND_INPUT_DATA_SIZE = 0x400,
			SOUND_INPUT_DATA_SAMPLES = (SOUND_INPUT_DATA_SIZE / 4),
		};

		class CSampleReader
		{
		public:
			CSampleReader();
			virtual ~CSampleReader() = default;

			void Reset();
			void SetMemory(uint8*, uint32);
			void SetSampleCache(CSpuSampleCache*);
			void SetIrqWatcher(CSpuIrqWatcher*);
			void SetDestinationSamplingRate(uint32);

			void LoadState(const CRegisterState&);
			void SaveState(CRegisterState&) const;

			void SetParamsRead(uint32, uint32);
			void SetParamsNoRead(uint32, uint32);
			void SetPitch(uint32, uint16);
			int32 GetSample();
			uint32 GetRepeat() const;
			void SetRepeat(uint32);
			uint32 GetCurrent() const;
			bool IsDone() const;
			void ClearIsDone();
			bool GetEndFlag() const;
			void ClearEndFlag();

			bool DidChangeRepeat() const;
			void ClearDidChangeRepeat();

		private:
			static constexpr int BUFFER_SAMPLES = 28;

			void SetParams(uint32, uint32);
			void UnpackSamples(int16*);
			void AdvanceBuffer();
			void UpdateSampleStep();

			uint8* m_ram = nullptr;
			uint32 m_ramSize = 0;
			CSpuSampleCache* m_sampleCache = nullptr;
			CSpuIrqWatcher* m_irqWatcher = nullptr;

			uint32 m_srcSampleIdx = 0;
			uint32 m_sampleStep = 0;
			uint32 m_srcSamplingRate = 0;
			uint32 m_dstSamplingRate = 0;
			uint32 m_nextSampleAddr = 0;
			uint32 m_repeatAddr = 0;
			int16 m_buffer[BUFFER_SAMPLES * 2];
			uint16 m_pitch;
			int32 m_s1;
			int32 m_s2;
			bool m_done;
			bool m_nextValid;
			bool m_endFlag;
			bool m_didChangeRepeat;

			static_assert((sizeof(decltype(m_buffer)) % 16) == 0, "sizeof(m_buffer) must be a multiple of 16 (needed for saved state).");
			static_assert(CSampleReader::BUFFER_SAMPLES == CSpuSampleCache::BUFFER_SAMPLES, "Buffer sample size must match cache sample size.");
		};

		class CBlockSampleReader
		{
		public:
			void Reset();
			void SetBaseSamplingRate(uint32);
			void SetDestinationSamplingRate(uint32);
			bool GetSpdifBypass();
			void SetSpdifBypass(bool);

			bool CanReadSamples() const;

			void FillBlock(const uint8*);
			void GetSamples(int32[2]);

		private:
			void UpdateSampleStep();

			uint32 m_baseSamplingRate = 0;
			uint32 m_dstSamplingRate = 0;
			uint32 m_srcSampleIdx = 0;
			uint32 m_sampleStep = 0;
			uint8 m_blockBuffer[SOUND_INPUT_DATA_SIZE];
			bool m_spdifBypass = false;
		};

		enum
		{
			MAX_ADSR_VOLUME = 0x7FFFFFFF,
		};

		void UpdateAdsr(CHANNEL&);
		void UpdateReverb(int16[2], int16*);
		uint32 GetAdsrDelta(unsigned int) const;
		float GetReverbSample(uint32) const;
		void SetReverbSample(uint32, float);
		uint32 GetReverbOffset(unsigned int) const;
		float GetReverbCoef(unsigned int) const;

		int32 ComputeChannelVolume(const CHANNEL_VOLUME&, int32);

		static const uint32 g_linearIncreaseSweepDeltas[0x80];
		static const uint32 g_linearDecreaseSweepDeltas[0x80];

		uint8* m_ram;
		uint32 m_ramSize;
		unsigned int m_spuNumber;
		uint32 m_baseSamplingRate;

		uint32 m_irqAddr = 0;
		bool m_irqPending = false;
		uint16 m_transferMode;
		uint32 m_transferAddr;
		uint32 m_core0OutputOffset;
		UNION32_16 m_channelOn;
		UNION32_16 m_channelReverb;
		uint32 m_reverbWorkAddrStart;
		uint32 m_reverbWorkAddrEnd;
		uint32 m_reverbCurrAddr;
		uint16 m_ctrl;
		int m_reverbTicks;
		uint32 m_reverb[REVERB_REG_COUNT];
		CSpuSampleCache* m_sampleCache = nullptr;
		CSpuIrqWatcher* m_irqWatcher = nullptr;
		CHANNEL m_channel[MAX_CHANNEL];
		CSampleReader m_reader[MAX_CHANNEL];
		uint32 m_adsrLogTable[160];
		bool m_reverbEnabled;
		float m_volumeAdjust;

		CBlockSampleReader m_blockReader;
		uint32 m_soundInputDataAddr = 0;
		uint32 m_blockWritePtr = 0;

		static_assert((sizeof(decltype(m_reverb)) % 16) == 0, "sizeof(m_reverb) must be a multiple of 16 (needed for saved state).");
	};
}
