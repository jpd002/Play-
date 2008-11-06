#ifndef _IOP_SPU_H_
#define _IOP_SPU_H_

#include "Iop_SpuBase.h"

namespace Iop
{
	class CSpu
	{
	public:
					CSpu(CSpuBase&);
		virtual		~CSpu();

		void		Reset();

		uint16		ReadRegister(uint32);
		void		WriteRegister(uint32, uint16);

		enum
		{
			SPU_BEGIN	= 0x1F801C00,
			SPU_END		= 0x1F801DFF
		};

		enum
		{
			CH0_BASE	= 0x1F801C00,
			CH1_BASE	= 0x1F801C10,
			CH2_BASE	= 0x1F801C20,
			CH3_BASE	= 0x1F801C30,
			CH4_BASE	= 0x1F801C40,
			CH5_BASE	= 0x1F801C50,
			CH6_BASE	= 0x1F801C60,
			CH7_BASE	= 0x1F801C70,
			CH8_BASE	= 0x1F801C80,
			CH9_BASE	= 0x1F801C90,
			CH10_BASE	= 0x1F801CA0,
			CH11_BASE	= 0x1F801CB0,
			CH12_BASE	= 0x1F801CC0,
			CH13_BASE	= 0x1F801CD0,
			CH14_BASE	= 0x1F801CE0,
			CH15_BASE	= 0x1F801CF0,
			CH16_BASE	= 0x1F801D00,
			CH17_BASE	= 0x1F801D10,
			CH18_BASE	= 0x1F801D20,
			CH19_BASE	= 0x1F801D30,
			CH20_BASE	= 0x1F801D40,
			CH21_BASE	= 0x1F801D50,
			CH22_BASE	= 0x1F801D60,
			CH23_BASE	= 0x1F801D70,
		};

		enum
		{
			CH_VOL_LEFT		= 0x00,
			CH_VOL_RIGHT	= 0x02,
			CH_PITCH		= 0x04,
			CH_ADDRESS		= 0x06,
			CH_ADSR_LEVEL	= 0x08,
			CH_ADSR_RATE	= 0x0A,
			CH_ADSR_VOLUME	= 0x0C,
			CH_REPEAT		= 0x0E
		};

		enum
		{
			SPU_GENERAL_BASE = 0x1F801D80,
		};

		enum
		{
			MAIN_VOL_LEFT	= 0x1F801D80,
			MAIN_VOL_RIGHT	= 0x1F801D82,
			REVERB_LEFT		= 0x1F801D84,
			REVERB_RIGHT	= 0x1F801D86,
			VOICE_ON_0		= 0x1F801D88,
			VOICE_ON_1		= 0x1F801D8A,
			VOICE_OFF_0		= 0x1F801D8C,
			VOICE_OFF_1		= 0x1F801D8E,
			FM_0			= 0x1F801D90,
			FM_1			= 0x1F801D92,
			NOISE_0			= 0x1F801D94,
			NOISE_1			= 0x1F801D96,
			REVERB_0		= 0x1F801D98,
			REVERB_1		= 0x1F801D9A,
			CHANNEL_ON_0	= 0x1F801D9C,
			CHANNEL_ON_1	= 0x1F801D9E,
			REVERB_WORK		= 0x1F801DA2,
			IRQ_ADDR		= 0x1F801DA4,
			BUFFER_ADDR		= 0x1F801DA6,
			SPU_DATA		= 0x1F801DA8,
			SPU_CTRL0		= 0x1F801DAA,
			SPU_STATUS0		= 0x1F801DAC,
			SPU_STATUS1		= 0x1F801DAE,
			CD_VOL_LEFT		= 0x1F801DB0,
			CD_VOL_RIGHT	= 0x1F801DB2,
			EXT_VOL_LEFT	= 0x1F801DB4,
			EXT_VOL_RIGHT	= 0x1F801DB6,
			REVERB_START	= 0x1F801DC0,
			REVERB_END		= 0x1F801E00,
		};

	private:
		void			DisassembleRead(uint32);
		void			DisassembleWrite(uint32, uint16);

		CSpuBase&		m_base;

		uint16			m_ctrl;
		uint16			m_status0;
		uint16			m_status1;
	};
}

#endif
