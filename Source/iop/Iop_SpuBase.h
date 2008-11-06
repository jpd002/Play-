#ifndef _IOP_SPUBASE_H_
#define _IOP_SPUBASE_H_

#include <boost/static_assert.hpp>
#include "Types.h"
#include "BasicUnions.h"
#include "convertible.h"

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
		BOOST_STATIC_ASSERT(sizeof(ADSR_LEVEL) >= sizeof(uint16));

		struct ADSR_RATE : public convertible<uint16>
		{
			unsigned int	releaseRate			: 5;
			unsigned int	releaseMode			: 1;
			unsigned int	sustainRate			: 7;
			unsigned int	reserved0			: 1;
			unsigned int	sustainDirection	: 1;
			unsigned int	sustainMode			: 1;
		};
		BOOST_STATIC_ASSERT(sizeof(ADSR_RATE) >= sizeof(uint16));

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
		BOOST_STATIC_ASSERT(sizeof(CHANNEL_VOLUME) >= sizeof(uint16));

	    enum
	    {
		    MAX_CHANNEL = 24
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
			uint16			pitch;
			uint32			address;
			ADSR_LEVEL		adsrLevel;
			ADSR_RATE		adsrRate;
			uint32			adsrVolume;
			uint32			repeat;
			uint16			status;
			uint32          current;
		};


                        CSpuBase(uint8*, uint32);
        virtual         ~CSpuBase();

	    void		    Reset();

        void		    SetBaseSamplingRate(uint32);

	    uint32		    GetTransferAddress() const;
	    void		    SetTransferAddress(uint32);

		void			SetReverbParam(unsigned int, uint16);
		void			SetReverbWorkAddressStart(uint32);
		void			SetReverbWorkAddressEnd(uint32);
		void			SetReverbCurrentAddress(uint32);

        UNION32_16	    GetChannelOn() const;
		void			SetChannelOn(uint16, uint16);
		void			SetChannelOnLo(uint16);
		void			SetChannelOnHi(uint16);

		UNION32_16	    GetChannelReverb() const;
		void			SetChannelReverb(uint16, uint16);
		void			SetChannelReverbLo(uint16);
		void			SetChannelReverbHi(uint16);

        CHANNEL&		GetChannel(unsigned int);

	    void		    SendKeyOn(uint32);
	    void		    SendKeyOff(uint32);

		void			WriteWord(uint16);

        uint32		    ReceiveDma(uint8*, uint32, uint32);

	    void		    Render(int16*, unsigned int, unsigned int);

    private:
	    enum
	    {
		    MAX_ADSR_VOLUME = 0x7FFFFFFF,
	    };

		void			UpdateAdsr(CHANNEL&);
		uint32			GetAdsrDelta(unsigned int) const;
		float			GetReverbSample(uint32) const;
		void			SetReverbSample(uint32, float);
		uint32			GetReverbOffset(unsigned int) const;
		float			GetReverbCoef(unsigned int) const;

		uint8*          m_ram;
        uint32          m_ramSize;
	    uint32			m_baseSamplingRate;
	    uint32			m_bufferAddr;
	    UNION32_16		m_channelOn;
	    UNION32_16		m_channelReverb;
	    uint32			m_reverbWorkAddrStart;
        uint32          m_reverbWorkAddrEnd;
	    uint32			m_reverbCurrAddr;
	    int				m_reverbTicks;
	    uint16			m_reverb[REVERB_REG_COUNT];
    	CHANNEL			m_channel[MAX_CHANNEL];
    	uint32			m_adsrLogTable[160];
	};
}

#endif
/*
		class CSampleReader
		{
		public:
							CSampleReader();
			virtual			~CSampleReader();

			void			Reset();

			void			SetParams(uint8*, uint8*);
			void			SetPitch(uint32, uint16);
			void			GetSamples(int16*, unsigned int, unsigned int);
			uint8*			GetRepeat() const;
			uint8*          GetCurrent() const;

		private:
			enum
			{
				BUFFER_SAMPLES = 28,
			};

			void			UnpackSamples(int16*);
			void			AdvanceBuffer();
			int16			GetSample(double);
			double			GetNextTime() const;
			double			GetBufferStep() const;
			double			GetSamplingRate() const;

			unsigned int	m_sourceSamplingRate;
			uint8*			m_nextSample;
			uint8*			m_repeat;
			int16			m_buffer[BUFFER_SAMPLES * 2];
			uint16			m_pitch;
			double			m_currentTime;
			double			m_dstTime;
			double			m_s1;
			double			m_s2;
			bool			m_done;
			bool			m_nextValid;
		};
*/