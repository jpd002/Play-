#ifndef _IPU_H_
#define _IPU_H_

#include "Types.h"
#include "BitStream.h"
#include "mpeg2/VLCTable.h"
#include "mpeg2/DctCoefficientTable.h"
#include <functional>
#include "MailBox.h"

class CIPU
{
public:
	typedef std::function<uint32 (void*, uint32)> Dma3ReceiveHandler;

						CIPU();
	virtual				~CIPU();

	enum REGISTER
	{
		IPU_CMD			= 0x10002000,
		IPU_CTRL		= 0x10002010,
		IPU_BP			= 0x10002020,
		IPU_TOP			= 0x10002030,
		IPU_OUT_FIFO	= 0x10007000,
		IPU_IN_FIFO		= 0x10007010,
	};

	void				Reset();
	uint32				GetRegister(uint32);
	void				SetRegister(uint32, uint32);
	void				SetDMA3ReceiveHandler(const Dma3ReceiveHandler&);
	uint32				ReceiveDMA4(uint32, uint32, bool, uint8*);

	void				ExecuteCommand();
	bool				WillExecuteCommand() const;

private:
	enum IPU_CTRL_BITS
	{
		IPU_CTRL_ECD = 0x00004000,
		IPU_CTRL_RST = 0x40000000,
	};

	class COUTFIFO
	{
	public:
							COUTFIFO();
		virtual				~COUTFIFO();

		void				Write(void*, unsigned int);
		void				Flush();
		void				SetReceiveHandler(const Dma3ReceiveHandler&);

		void				Reset();

	private:
		void				RequestGrow(unsigned int);

		enum GROWSIZE
		{
			GROWSIZE = 0x200,
		};

		unsigned int		m_size;
		unsigned int		m_alloc;
		uint8*				m_buffer;
		Dma3ReceiveHandler	m_receiveHandler;
	};

	class CINFIFO : public Framework::CBitStream
	{
	public:
							CINFIFO();
		virtual				~CINFIFO();

		void				Write(void*, unsigned int);

		void				Advance(uint8) override;
		uint8				GetBitIndex() const override;

		bool				TryPeekBits_LSBF(uint8, uint32&) override;
		bool				TryPeekBits_MSBF(uint8, uint32&) override;

		void				SetBitPosition(unsigned int);
		unsigned int		GetSize();
		unsigned int		GetAvailableBits();
		void				Reset();

		enum BUFFERSIZE
		{
			BUFFERSIZE = 0xF0,
		};

	private:
		void				SyncLookupBits();

		uint8				m_buffer[BUFFERSIZE];
		uint64				m_lookupBits;
		bool				m_lookupBitsDirty;
		unsigned int		m_size;
		unsigned int		m_bitPosition;
	};

	class CCommand
	{
	public:
		virtual			~CCommand() {}
		virtual void	Execute() = 0;

	private:

	};

	//0x00 ------------------------------------------------------------
	class CBCLRCommand : public CCommand
	{
	public:
						CBCLRCommand();
		void			Initialize(CINFIFO*, uint32);
		void			Execute();

	private:
		CINFIFO*		m_IN_FIFO;
		uint32			m_commandCode;
	};

	//0x02 ------------------------------------------------------------
	class CBDECCommand_ReadDcDiff : public CCommand
	{
	public:
										CBDECCommand_ReadDcDiff();

		void							Initialize(CINFIFO*, unsigned int, int16*);
		void							Execute();

	private:
		enum STATE
		{
			STATE_READSIZE,
			STATE_READDIFF,
			STATE_DONE
		};

		STATE							m_state;
		CINFIFO*						m_IN_FIFO;
		unsigned int					m_channelId;
		uint8							m_dcSize;
		int16*							m_result;
	};

	class CBDECCommand_ReadDct : public CCommand
	{
	public:
										CBDECCommand_ReadDct();

		void							Initialize(CINFIFO*, int16* block, unsigned int channelId, int16* dcPredictor, bool mbi, bool isMpeg1CoeffVLCTable, bool isMpeg2);
		void							Execute();

	private:
		enum STATE
		{
			STATE_INIT,
			STATE_READDCDIFF,
			STATE_CHECKEOB,
			STATE_READCOEFF,
			STATE_SKIPEOB
		};

		CINFIFO*						m_IN_FIFO;
		STATE							m_state;
		int16*							m_block;
		unsigned int					m_channelId;
		bool							m_mbi;
		bool							m_isMpeg1CoeffVLCTable;
		bool							m_isMpeg2;
		unsigned int					m_blockIndex;
		MPEG2::CDctCoefficientTable*	m_coeffTable;
		int16*							m_dcPredictor;
		int16							m_dcDiff;
		CBDECCommand_ReadDcDiff			m_readDcDiffCommand;
	};

	class CBDECCommand : public CCommand
	{
	public:
		struct CONTEXT
		{
			bool	isMpeg1CoeffVLCTable;
			bool	isMpeg2;
			bool	isLinearQScale;
			bool	isZigZag;
			uint8*	intraIq;
			uint8*	nonIntraIq;
			int16*	dcPredictor;
			uint32	dcPrecision;
		};
										CBDECCommand();

		void							Initialize(CINFIFO*, COUTFIFO*, uint32, const CONTEXT&);
		void							Execute();

	private:
		enum STATE
		{
			STATE_ADVANCE,
			STATE_READCBP,
			STATE_RESETDC,
			STATE_DECODEBLOCK_BEGIN,
			STATE_DECODEBLOCK_READCOEFFS,
			STATE_DECODEBLOCK_GOTONEXT,
			STATE_DONE
		};

		struct BLOCKENTRY
		{
			int16*						block;
			unsigned int				channel;
		};

		CINFIFO*						m_IN_FIFO;
		COUTFIFO*						m_OUT_FIFO;
		STATE							m_state;

		bool							m_mbi;
		bool							m_dcr;
		bool							m_dt;
		uint8							m_qsc;
		uint8							m_fb;

		uint8							m_codedBlockPattern;

		BLOCKENTRY						m_blocks[6];

		int16							m_yBlock[4][64];
		int16							m_cbBlock[64];
		int16							m_crBlock[64];

		unsigned int					m_currentBlockIndex;

		CONTEXT							m_context;
		CBDECCommand_ReadDct			m_readDctCoeffsCommand;
	};

	//0x03 ------------------------------------------------------------
	class CVDECCommand : public CCommand
	{
	public:
							CVDECCommand();

		void				Initialize(CINFIFO*, uint32, uint32, uint32*);
		void				Execute();

	private:
		enum STATE
		{
			STATE_ADVANCE,
			STATE_DECODE,
			STATE_DONE
		};

		uint32				m_commandCode;
		uint32*				m_result;
		CINFIFO*			m_IN_FIFO;
		STATE				m_state;
		MPEG2::CVLCTable*	m_table;
	};

	//0x04 ------------------------------------------------------------
	class CFDECCommand : public CCommand
	{
	public:
						CFDECCommand();

		void			Initialize(CINFIFO*, uint32, uint32*);
		void			Execute();

	private:
		enum STATE
		{
			STATE_ADVANCE,
			STATE_DECODE,
			STATE_DONE
		};

		uint32			m_commandCode;
		uint32*			m_result;
		CINFIFO*		m_IN_FIFO;
		STATE			m_state;
	};

	//0x05 ------------------------------------------------------------
	class CSETIQCommand : public CCommand
	{
	public:
						CSETIQCommand();

		void			Initialize(CINFIFO*, uint8*);
		void			Execute();

	private:
		CINFIFO*		m_IN_FIFO;
		uint8*			m_matrix;
		unsigned int	m_currentIndex;
	};

	//0x06 ------------------------------------------------------------
	class CSETVQCommand : public CCommand
	{
	public:
						CSETVQCommand();

		void			Initialize(CINFIFO*, uint16*);
		void			Execute();

	private:
		CINFIFO*		m_IN_FIFO;
		uint16*			m_clut;
		unsigned int	m_currentIndex;
	};

	//0x07 ------------------------------------------------------------
	class CCSCCommand : public CCommand
	{
	public:
						CCSCCommand();

		void			Initialize(CINFIFO*, COUTFIFO*, uint32);
		void			Execute();

	private:
		enum STATE
		{
			STATE_READBLOCKSTART,
			STATE_READBLOCK,
			STATE_CONVERTBLOCK,
			STATE_DONE,
		};

		enum
		{
			BLOCK_SIZE = 0x180,
		};

		void			GenerateCbCrMap();

		CINFIFO*		m_IN_FIFO;
		COUTFIFO*		m_OUT_FIFO;
		uint8			m_OFM;
		uint8			m_DTE;
		uint16			m_MBC;
		unsigned int	m_currentIndex;
		STATE			m_state;

		unsigned int	m_nCbCrMap[0x100];

		uint8			m_block[BLOCK_SIZE];
	};

	//0x09 ------------------------------------------------------------
	class CSETTHCommand : public CCommand
	{
	public:
						CSETTHCommand();

		void			Initialize(uint32, uint16*, uint16*);
		void			Execute();

	private:
		uint32			m_commandCode;
		uint16*			m_TH0;
		uint16*			m_TH1;
	};

	void						InitializeCommand(uint32);

	uint32						GetPictureType();
	uint32						GetDcPrecision();
	bool						GetIsMPEG2();
	bool						GetIsLinearQScale();
	bool						GetIsZigZagScan();
	bool						GetIsMPEG1CoeffVLCTable();

	static void					DequantiseBlock(int16*, uint8, uint8, bool isLinearQScale, uint32 dcPrecision, uint8* intraIq, uint8* nonIntraIq);
	static void					InverseScan(int16*, bool isZigZag);

	uint32						GetBusyBit(bool) const;

	void						DisassembleGet(uint32);
	void						DisassembleSet(uint32, uint32);
	void						DisassembleCommand(uint32);

	uint8						m_nIntraIQ[0x40];
	uint8						m_nNonIntraIQ[0x40];
	uint16						m_nVQCLUT[0x10];
	uint16						m_nTH0;
	uint16						m_nTH1;

	int16						m_nDcPredictor[3];

	uint32						m_IPU_CMD[2];
	uint32						m_IPU_CTRL;
	COUTFIFO					m_OUT_FIFO;
	CINFIFO						m_IN_FIFO;
	bool						m_isBusy;

	CCommand*					m_currentCmd;
	CBCLRCommand				m_BCLRCommand;
	CBDECCommand				m_BDECCommand;
	CVDECCommand				m_VDECCommand;
	CFDECCommand				m_FDECCommand;
	CSETIQCommand				m_SETIQCommand;
	CSETVQCommand				m_SETVQCommand;
	CCSCCommand					m_CSCCommand;
	CSETTHCommand				m_SETTHCommand;
};

#endif
