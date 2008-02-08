#ifndef _IPU_H_
#define _IPU_H_

#include "Types.h"
#include "BitStream.h"
#include <boost/thread.hpp>
#include <functional>
#include "MailBox.h"

class CIPU
{
public:
    typedef std::tr1::function<uint32 (void*, uint32)> Dma3ReceiveHandler;

                        CIPU();
    virtual             ~CIPU();

	enum REGISTER
	{
		IPU_CMD			= 0x10002000,
		IPU_CTRL		= 0x10002010,
		IPU_BP			= 0x10002020,
		IPU_TOP			= 0x10002030,
		IPU_OUT_FIFO	= 0x10007000,
		IPU_IN_FIFO		= 0x10007010,
	};

	void                Reset();
	uint32              GetRegister(uint32);
	void                SetRegister(uint32, uint32);
    void                SetDMA3ReceiveHandler(const Dma3ReceiveHandler&);
    uint32              ReceiveDMA4(uint32, uint32, bool, uint8*);
//	void                DMASliceDoneCallback();

private:
	class COutFifoBase
	{
	public:
		virtual			~COutFifoBase();
		virtual void	Write(void*, unsigned int) = 0;
		virtual void	Flush() = 0;
	};

	class COUTFIFO : public COutFifoBase
	{
	public:
						COUTFIFO();
		virtual			~COUTFIFO();
		virtual void	Write(void*, unsigned int);
		virtual void	Flush();
        void            SetReceiveHandler(const Dma3ReceiveHandler&);

	private:
		void			RequestGrow(unsigned int);

		enum GROWSIZE
		{
			GROWSIZE = 0x200,
		};

		unsigned int	    m_nSize;
		unsigned int	    m_nAlloc;
		uint8*			    m_pBuffer;
        Dma3ReceiveHandler  m_receiveHandler;
	};

	class CIDecFifo : public Framework::CBitStream, public COutFifoBase
	{
	public:
						CIDecFifo();
		virtual			~CIDecFifo();
		void			Reset();
		virtual void	Write(void*, unsigned int);
		virtual void	Flush();
		virtual uint32	GetBits_LSBF(uint8);
		virtual uint32	GetBits_MSBF(uint8);
		virtual uint32	PeekBits_LSBF(uint8);
		virtual uint32	PeekBits_MSBF(uint8);
		virtual void	SeekToByteAlign();
		virtual bool	IsOnByteBoundary();

	private:
		enum BUFFERSIZE
		{
			BUFFERSIZE = 0x300,
		};

		uint8			m_nBuffer[BUFFERSIZE];
		unsigned int	m_nReadPtr;
		unsigned int	m_nWritePtr;
	};

	class CINFIFO : public Framework::CBitStream
	{
	public:
                        CINFIFO();
		virtual         ~CINFIFO();
		void            Write(void*, unsigned int);
		uint32          GetBits_MSBF(uint8);
		uint32          GetBits_LSBF(uint8);
		uint32          PeekBits_LSBF(uint8);
		uint32          PeekBits_MSBF(uint8);
		void            SkipBits(uint8);
		void			SeekToByteAlign();
		bool            IsOnByteBoundary();
		unsigned int    GetBitPosition();
		void            SetBitPosition(unsigned int);
		unsigned int    GetSize();
		void            Reset();

		enum BUFFERSIZE
		{
			BUFFERSIZE = 0xF0,
		};

    private:
        uint8			    m_nBuffer[BUFFERSIZE];
		unsigned int        m_nSize;
		unsigned int        m_nBitPosition;
        boost::mutex        m_accessMutex;
        boost::condition    m_dataNeededCondition;
	};

    void                CommandThread();
    void                ExecuteCommand(uint32);
    void                DecodeIntra(uint8, uint8, uint8, uint8, uint8, uint8);
    void                DecodeBlock(COutFifoBase*, uint8, uint8, uint8, uint8, uint8);
    void                VariableLengthDecode(uint8, uint8);
    void                FixedLengthDecode(uint8);
    void                LoadIQMatrix(uint8*);
    void                LoadVQCLUT();
    void                ColorSpaceConversion(Framework::CBitStream*, uint8, uint8, uint16);
    void                SetThresholdValues(uint32);

//    bool                IsExecutionRisky(unsigned int);

    uint32              GetPictureType();
    uint32              GetDcPrecision();
    bool                GetIsMPEG2();
    bool                GetIsLinearQScale();
    bool                GetIsZigZagScan();
    bool                GetIsMPEG1CoeffVLCTable();

    void                DecodeDctCoefficients(unsigned int, int16*, uint8);
    void                DequantiseBlock(int16*, uint8, uint8);
    void                InverseScan(int16*);
    int16               GetDcDifferential(unsigned int);

    void                GenerateCbCrMap();

    uint32              GetBusyBit(bool) const;

    void                DisassembleGet(uint32);
    void                DisassembleSet(uint32, uint32);
    void                DisassembleCommand(uint32);

    unsigned int        m_nCbCrMap[0x100];

    uint8               m_nIntraIQ[0x40];
    uint8               m_nNonIntraIQ[0x40];
    uint16              m_nVQCLUT[0x10];
    uint16              m_nTH0;
    uint16              m_nTH1;

    int16               m_nDcPredictor[3];

    uint32              m_IPU_CMD[2];
    uint32              m_IPU_CTRL;
    COUTFIFO            m_OUT_FIFO;
    CINFIFO             m_IN_FIFO;
    boost::thread*      m_cmdThread;
    CMailBox            m_cmdThreadMail;
    bool                m_isBusy;
    bool                m_busyWhileReadingCMD;
    bool                m_busyWhileReadingTOP;
};

#endif
