#ifndef _IPU_H_
#define _IPU_H_

#include "Types.h"
#include "BitStream.h"

class CIPU
{
public:
	enum REGISTER
	{
		IPU_CMD			= 0x10002000,
		IPU_CTRL		= 0x10002010,
		IPU_BP			= 0x10002020,
		IPU_TOP			= 0x10002030,
		IPU_OUT_FIFO	= 0x10007000,
		IPU_IN_FIFO		= 0x10007010,
	};

	static void			Reset();
	static uint32		GetRegister(uint32);
	static void			SetRegister(uint32, uint32);
	static uint32		ReceiveDMA(uint32, uint32, bool);
	static void			DMASliceDoneCallback();

private:
	class COUTFIFO
	{
	public:
						COUTFIFO();
		virtual			~COUTFIFO();
		void			Write(void*, unsigned int);
		void			Flush();

	private:
		void			RequestGrow(unsigned int);

		enum GROWSIZE
		{
			GROWSIZE = 0x200,
		};

		unsigned int	m_nSize;
		unsigned int	m_nAlloc;
		uint8*			m_pBuffer;
	};

	class CINFIFO : public Framework::CBitStream
	{
	public:
						CINFIFO();
		virtual			~CINFIFO();
		void			Write(void*, unsigned int);
		uint32			GetBits_MSBF(uint8);
		uint32			GetBits_LSBF(uint8);
		uint32			PeekBits_LSBF(uint8);
		uint32			PeekBits_MSBF(uint8);
		void			SkipBits(uint8);
		void			SeekToByteAlign();
		bool			IsOnByteBoundary();
		unsigned int	GetBitPosition();
		void			SetBitPosition(unsigned int);
		unsigned int	GetSize();
		void			Reset();

		enum BUFFERSIZE
		{
			BUFFERSIZE = 0xF0,
		};

	private:
		uint8			m_nBuffer[BUFFERSIZE];

		unsigned int	m_nSize;
		unsigned int	m_nBitPosition;
	};

	static void			ExecuteCommand(uint32);
	static void			DecodeIntra(uint8, uint8, uint8, uint8, uint8, uint8);
	static void			DecodeBlock(uint8, uint8, uint8, uint8, uint8);
	static void			VariableLengthDecode(uint8, uint8);
	static void			FixedLengthDecode(uint8);
	static void			LoadIQMatrix(uint8*);
	static void			LoadVQCLUT();
	static void			ColorSpaceConversion(uint8, uint8, uint16);
	static void			SetThresholdValues(uint32);
	
	static bool			IsExecutionRisky(unsigned int);

	static uint32		GetPictureType();
	static uint32		GetDcPrecision();
	static bool			GetIsMPEG2();
	static bool			GetIsLinearQScale();
	static bool			GetIsZigZagScan();
	static bool			GetIsMPEG1CoeffVLCTable();

	static void			DecodeDctCoefficients(unsigned int, int16*, uint8);
	static void			DequantiseBlock(int16*, uint8, uint8);
	static void			InverseScan(int16*);
	static int16		GetDcDifferential(unsigned int);

	static void			GenerateCbCrMap();

	static void			DisassembleGet(uint32);
	static void			DisassembleSet(uint32, uint32);
	static void			DisassembleCommand(uint32);

	static uint32		m_nPendingCommand;

	static unsigned int m_nCbCrMap[0x100];

	static uint8		m_nIntraIQ[0x40];
	static uint8		m_nNonIntraIQ[0x40];
	static uint16		m_nVQCLUT[0x10];
	static uint16		m_nTH0;
	static uint16		m_nTH1;

	static int16		m_nDcPredictor[3];

	static uint32		m_IPU_CMD[2];
	static uint32		m_IPU_CTRL;
	static COUTFIFO		m_OUT_FIFO;
	static CINFIFO		m_IN_FIFO;
};

#endif
