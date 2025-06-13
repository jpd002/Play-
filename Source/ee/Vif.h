#pragma once

#include <climits>
#include <cstring>
#include "Types.h"
#include "Convertible.h"
#include "Vpu.h"
#include "../uint128.h"
#include "../Profiler.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "SimdDefs.h"

#ifdef FRAMEWORK_SIMD_USE_SSE
#include <emmintrin.h>
#endif

#ifdef _MSC_VER
#define FRAMEWORK_SAFE_BUFFERS __declspec(safebuffers)
#else
#define FRAMEWORK_SAFE_BUFFERS
#endif

class CINTC;

class CVif
{
public:
	enum
	{
		REGS0_START = 0x10003800,
		VIF0_STAT = 0x10003800,
		VIF0_FBRST = 0x10003810,
		VIF0_ERR = 0x10003820,
		VIF0_MARK = 0x10003830,
		VIF0_CYCLE = 0x10003840,
		VIF0_MODE = 0x10003850,
		VIF0_NUM = 0x10003860,
		VIF0_MASK = 0x10003870,
		VIF0_CODE = 0x10003880,
		VIF0_R0 = 0x10003900,
		VIF0_R1 = 0x10003910,
		VIF0_R2 = 0x10003920,
		VIF0_R3 = 0x10003930,
		REGS0_END = 0x10003A00,

		REGS1_START = 0x10003C00,
		VIF1_STAT = 0x10003C00,
		VIF1_FBRST = 0x10003C10,
		VIF1_ERR = 0x10003C20,
		VIF1_MARK = 0x10003C30,
		VIF1_CYCLE = 0x10003C40,
		VIF1_MODE = 0x10003C50,
		VIF1_NUM = 0x10003C60,
		VIF1_MASK = 0x10003C70,
		VIF1_CODE = 0x10003C80,
		VIF1_R0 = 0x10003D00,
		VIF1_R1 = 0x10003D10,
		VIF1_R2 = 0x10003D20,
		VIF1_R3 = 0x10003D30,
		REGS1_END = 0x10003E00,

		VIF0_FIFO_START = 0x10004000,
		VIF0_FIFO_END = 0x10004FFF,
		VIF1_FIFO_START = 0x10005000,
		VIF1_FIFO_END = 0x10005FFF,
	};

	CVif(unsigned int, CVpu&, CINTC&, uint8*, uint8*);
	virtual ~CVif() = default;

	virtual void Reset();
	uint32 GetRegister(uint32);
	void SetRegister(uint32, uint32);
	void CountTicks(uint32);
	virtual void SaveState(Framework::CZipArchiveWriter&);
	virtual void LoadState(Framework::CZipArchiveReader&);

	virtual uint32 GetTOP() const;
	virtual uint32 GetITOP() const;

	virtual uint32 ReceiveDMA(uint32, uint32, uint32, bool);

	bool IsWaitingForProgramEnd() const;

protected:
	enum
	{
		CODE_CMD_NOP = 0x00,
		CODE_CMD_STCYCL = 0x01,
		CODE_CMD_OFFSET = 0x02, //VIF1 only
		CODE_CMD_BASE = 0x03,   //VIF1 only
		CODE_CMD_ITOP = 0x04,
		CODE_CMD_STMOD = 0x05,
		CODE_CMD_MSKPATH3 = 0x06, //VIF1 only
		CODE_CMD_MARK = 0x07,
		CODE_CMD_FLUSHE = 0x10,
		CODE_CMD_FLUSH = 0x11,  //VIF1 only
		CODE_CMD_FLUSHA = 0x13, //VIF1 only
		CODE_CMD_MSCAL = 0x14,
		CODE_CMD_MSCALF = 0x15,
		CODE_CMD_MSCNT = 0x17,
		CODE_CMD_STMASK = 0x20,
		CODE_CMD_STROW = 0x30,
		CODE_CMD_STCOL = 0x31,
		CODE_CMD_MPG = 0x4A,
		CODE_CMD_DIRECT = 0x50,   //VIF1 only
		CODE_CMD_DIRECTHL = 0x51, //VIF1 only
	};

	enum
	{
		FBRST_RST = 0x01,
		FBRST_FBK = 0x02,
		FBRST_STP = 0x04,
		FBRST_STC = 0x08
	};

	enum
	{
		STAT_VPS_IDLE = 0x00,
		STAT_VPS_WAITING = 0x01,
		STAT_VPS_DECODING = 0x02,
		STAT_VPS_TRANSFER = 0x03,
		STAT_VPS_MASK = 0x03,

		STAT_FDR = 0x00800000,
	};

	enum
	{
		FIFO_SIZE = 0x100
	};

	class CFifoStream
	{
	public:
		CFifoStream(uint8*, uint8*);
		virtual ~CFifoStream() = default;

		void Reset();

		inline uint32 GetAvailableReadBytes() const
		{
			return GetRemainingDmaTransferSize() + (BUFFERSIZE - m_bufferPosition);
		}

		inline uint32 GetRemainingDmaTransferSize() const
		{
			return m_endAddress - m_nextAddress;
		}

		inline void Read(void* buffer, uint32 size)
		{
			assert(m_source != nullptr);
			assert(buffer != nullptr);
			uint8* readBuffer = reinterpret_cast<uint8*>(buffer);
			while(size != 0)
			{
				SyncBuffer();
				uint32 read = std::min<uint32>(size, BUFFERSIZE - m_bufferPosition);
				memcpy(readBuffer, reinterpret_cast<uint8*>(&m_buffer) + m_bufferPosition, read);
				readBuffer += read;
				m_bufferPosition += read;
				size -= read;
			}
		}

		template <size_t ValueSize>
		inline FRAMEWORK_SAFE_BUFFERS void ReadValue(void* buffer)
		{
			//Hyper-specialized version of Read that allows the compiler to better
			//optimize the code. Will only work with reads of at most 16 bytes.
			static_assert(ValueSize <= 0x10);
			assert(m_source != nullptr);
			assert(buffer != nullptr);
			uint32 availableBufferSize = BUFFERSIZE - m_bufferPosition;
			uint8* readBuffer = reinterpret_cast<uint8*>(buffer);
			if(ValueSize <= availableBufferSize)
			{
				memcpy(readBuffer, reinterpret_cast<uint8*>(&m_buffer) + m_bufferPosition, ValueSize);
				m_bufferPosition += ValueSize;
			}
			else if(!m_tagIncluded || (ValueSize <= 8))
			{
				uint128 qw[2];
				qw[0] = m_buffer;
				uint32 startPosition = m_bufferPosition;
				bool hasTag = m_tagIncluded;
				m_bufferPosition = BUFFERSIZE;
				SyncBuffer();
				qw[1] = m_buffer;
				if(hasTag)
				{
					qw[1].nD0 = qw[1].nD1;
				}
				m_bufferPosition += (ValueSize - availableBufferSize);
				memcpy(readBuffer, reinterpret_cast<uint8*>(&qw) + startPosition, ValueSize);
			}
			else
			{
				Read(buffer, ValueSize);
			}
		}

		void Flush();
		inline void Align32();
		void SetDmaParams(uint32, uint32, bool);
		void SetFifoParams(uint8*, uint32);

		uint8* GetDirectPointer() const;
		void Advance(uint32);

		uint128 GetBuffer() const;
		void SetBuffer(uint128);

		uint32 GetBufferPosition() const;
		void SetBufferPosition(uint32);

	private:
		inline void SyncBuffer()
		{
			assert(m_bufferPosition <= BUFFERSIZE);
			if(m_bufferPosition >= BUFFERSIZE)
			{
				assert(m_nextAddress < m_endAddress);
				m_buffer = *reinterpret_cast<uint128*>(&m_source[m_nextAddress]);
				m_nextAddress += 0x10;
				m_bufferPosition = 0;
				if(m_tagIncluded)
				{
					//Skip next 8 bytes
					m_tagIncluded = false;
					m_bufferPosition += 8;
				}
			}
		}

		enum
		{
			BUFFERSIZE = 0x10
		};

		uint8* m_ram = nullptr;
		uint8* m_spr = nullptr;

		alignas(16) uint128 m_buffer = {};
		uint32 m_bufferPosition = BUFFERSIZE;
		uint32 m_startAddress = 0;
		uint32 m_nextAddress = 0;
		uint32 m_endAddress = 0;
		bool m_tagIncluded = false;
		uint8* m_source = nullptr;
	};

	typedef CFifoStream StreamType;

	struct STAT : public convertible<uint32>
	{
		unsigned int nVPS : 2;
		unsigned int nVEW : 1;
		unsigned int nVGW : 1; //VIF1 only
		unsigned int nReserved0 : 2;
		unsigned int nMRK : 1;
		unsigned int nDBF : 1;
		unsigned int nVSS : 1;
		unsigned int nVFS : 1;
		unsigned int nVIS : 1;
		unsigned int nINT : 1;
		unsigned int nER0 : 1;
		unsigned int nER1 : 1;
		unsigned int nReserved2 : 9;
		unsigned int nFDR : 1; //VIF1 only
		unsigned int nFQC : 4;
		unsigned int nReserved3 : 4;
	};
	static_assert(sizeof(STAT) == sizeof(uint32), "Size of STAT struct must be 4 bytes.");

	struct ERR : public convertible<uint32>
	{
		unsigned int nMII : 1;
		unsigned int nME0 : 1;
		unsigned int nME1 : 1;
		unsigned int nReserved : 29;
	};
	static_assert(sizeof(ERR) == sizeof(uint32), "Size of ERR struct must be 4 bytes.");

	struct CYCLE : public convertible<uint32>
	{
		unsigned int nCL : 8;
		unsigned int nWL : 8;
		unsigned int reserved : 16;
	};
	static_assert(sizeof(CYCLE) == sizeof(uint32), "Size of CYCLE struct must be 4 bytes.");

	struct CODE : public convertible<uint32>
	{
		unsigned int nIMM : 16;
		unsigned int nNUM : 8;
		unsigned int nCMD : 7;
		unsigned int nI : 1;
	};
	static_assert(sizeof(CODE) == sizeof(uint32), "Size of CODE struct must be 4 bytes.");

	enum ADDMODE
	{
		MODE_NORMAL = 0,
		MODE_OFFSET = 1,
		MODE_DIFFERENCE = 2
	};

	enum MASKOP
	{
		MASK_DATA = 0,
		MASK_ROW = 1,
		MASK_COL = 2,
		MASK_MASK = 3
	};

	void ProcessFifoWrite(uint32, uint32);

	void ProcessPacket(StreamType&);
	virtual void ExecuteCommand(StreamType&, CODE);
	virtual void Cmd_UNPACK(StreamType&, CODE, uint32);

	void Cmd_MPG(StreamType&, CODE);
	void Cmd_STROW(StreamType&, CODE);
	void Cmd_STCOL(StreamType&, CODE);
	void Cmd_STMASK(StreamType&, CODE);

	inline uint32 GetColMaskOp(unsigned int) const;

	inline bool Unpack_S32(StreamType& stream, uint128& result)
	{
		if(stream.GetAvailableReadBytes() < 4) return false;

		uint32 value;
		stream.ReadValue<4>(&value);

		for(unsigned int i = 0; i < 4; i++)
		{
			result.nV[i] = value;
		}

		return true;
	}

	template <bool zeroExtend>
	inline bool Unpack_S16(StreamType& stream, uint128& result)
	{
		if(stream.GetAvailableReadBytes() < 2) return false;

		uint16 value;
		stream.ReadValue<2>(&value);

		uint32 temp = value;
		if(!zeroExtend)
		{
			temp = static_cast<int16>(temp);
		}

		for(unsigned int i = 0; i < 4; i++)
		{
			result.nV[i] = temp;
		}

		return true;
	}

	template <bool zeroExtend>
	inline bool Unpack_S8(StreamType& stream, uint128& result)
	{
		if(stream.GetAvailableReadBytes() < 1) return false;

		uint8 value;
		stream.ReadValue<1>(&value);

		uint32 temp = value;
		if(!zeroExtend)
		{
			temp = static_cast<int8>(temp);
		}

		for(unsigned int i = 0; i < 4; i++)
		{
			result.nV[i] = temp;
		}

		return true;
	}

	inline bool Unpack_V45(StreamType& stream, uint128& result)
	{
		if(stream.GetAvailableReadBytes() < 2) return false;

		uint16 value = 0;
		stream.ReadValue<2>(&value);

		result.nV0 = ((value >> 0) & 0x1F) << 3;
		result.nV1 = ((value >> 5) & 0x1F) << 3;
		result.nV2 = ((value >> 10) & 0x1F) << 3;
		result.nV3 = ((value >> 15) & 0x01) << 7;

		return true;
	}

	template <unsigned int fields>
	inline bool Unpack_V32(StreamType& stream, uint128& result)
	{
		if(stream.GetAvailableReadBytes() < (fields * 4)) return false;
		stream.ReadValue<fields * 4>(&result);
		return true;
	}

	template <unsigned int fields, bool zeroExtend>
	inline bool Unpack_V16(StreamType& stream, uint128& result)
	{
		if(stream.GetAvailableReadBytes() < (fields * 2)) return false;

		uint16 values[fields];
		stream.ReadValue<fields * 2>(values);

		for(unsigned int i = 0; i < fields; i++)
		{
			uint32 temp = values[i];
			if(!zeroExtend)
			{
				temp = static_cast<int16>(temp);
			}

			result.nV[i] = temp;
		}

		return true;
	}

	template <unsigned int fields, bool zeroExtend>
	inline bool Unpack_V8(StreamType& stream, uint128& result)
	{
		if(stream.GetAvailableReadBytes() < (fields)) return false;

		uint8 values[fields];
		stream.ReadValue<fields>(values);

		for(unsigned int i = 0; i < fields; i++)
		{
			uint32 temp = values[i];
			if(!zeroExtend)
			{
				temp = static_cast<int8>(temp);
			}

			result.nV[i] = temp;
		}

		return true;
	}

	template <uint8 dataType, bool usn>
	bool Unpack_ReadValue(StreamType& stream, uint128& writeValue)
	{
		bool success = false;
		switch(dataType)
		{
		case 0x00:
			//S-32
			success = Unpack_S32(stream, writeValue);
			break;
		case 0x01:
			//S-16
			success = Unpack_S16<usn>(stream, writeValue);
			break;
		case 0x02:
			//S-8
			success = Unpack_S8<usn>(stream, writeValue);
			break;
		case 0x04:
			//V2-32
			success = Unpack_V32<2>(stream, writeValue);
			break;
		case 0x05:
			//V2-16
			success = Unpack_V16<2, usn>(stream, writeValue);
			break;
		case 0x06:
			//V2-8
			success = Unpack_V8<2, usn>(stream, writeValue);
			break;
		case 0x08:
			//V3-32
			success = Unpack_V32<3>(stream, writeValue);
			break;
		case 0x09:
			//V3-16
			success = Unpack_V16<3, usn>(stream, writeValue);
			break;
		case 0x0A:
			//V3-8
			success = Unpack_V8<3, usn>(stream, writeValue);
			break;
		case 0x0C:
			//V4-32
			success = Unpack_V32<4>(stream, writeValue);
			break;
		case 0x0D:
			//V4-16
			success = Unpack_V16<4, usn>(stream, writeValue);
			break;
		case 0x0E:
			//V4-8
			success = Unpack_V8<4, usn>(stream, writeValue);
			break;
		case 0x0F:
			//V4-5
			success = Unpack_V45(stream, writeValue);
			break;
		default:
			assert(0);
			break;
		}
		return success;
	}

	template <uint8 dataType, bool clGreaterEqualWl, bool useMask, uint8 mode, bool usn>
	void Unpack(StreamType& stream, CODE nCommand, uint32 nDstAddr)
	{
		assert((nCommand.nCMD & 0x60) == 0x60);

		const auto vuMem = m_vpu.GetVuMemory();
		const auto vuMemSize = m_vpu.GetVuMemorySize();
		uint32 cl = m_CYCLE.nCL;
		uint32 wl = m_CYCLE.nWL;
		if(wl == 0)
		{
			wl = UINT_MAX;
			cl = 0;
		}

		if(m_NUM == nCommand.nNUM)
		{
			m_readTick = 0;
			m_writeTick = 0;
		}

		uint32 currentNum = (m_NUM == 0) ? 256 : m_NUM;
		uint32 codeNum = (m_CODE.nNUM == 0) ? 256 : m_CODE.nNUM;
		uint32 transfered = codeNum - currentNum;

		if(cl > wl)
		{
			nDstAddr += cl * (transfered / wl) + (transfered % wl);
		}
		else
		{
			nDstAddr += transfered;
		}

		nDstAddr *= 0x10;
		assert(nDstAddr < vuMemSize);
		nDstAddr &= (vuMemSize - 1);

		while(currentNum != 0)
		{
			bool mustWrite = false;
			uint128 writeValue;
			memset(&writeValue, 0, sizeof(writeValue));

			if(clGreaterEqualWl)
			{
				if(m_readTick < wl)
				{
					bool success = Unpack_ReadValue<dataType, usn>(stream, writeValue);
					if(!success) break;
					mustWrite = true;
				}
			}
			else
			{
				if(m_writeTick < cl)
				{
					bool success = Unpack_ReadValue<dataType, usn>(stream, writeValue);
					if(!success) break;
				}

				mustWrite = true;
			}

			if(mustWrite)
			{
				auto dst = reinterpret_cast<uint128*>(vuMem + nDstAddr);
				uint32 col = (m_writeTick > 3) ? 3 : m_writeTick;
				uint32 colMask = useMask ? GetColMaskOp(col) : 0;

				if((colMask == 0) && (mode == 0))
				{
					(*dst) = writeValue;
				}
#ifdef FRAMEWORK_SIMD_USE_SSE
				else if((colMask == 0) && (mode == MODE_OFFSET))
				{
					*reinterpret_cast<__m128i*>(dst) = _mm_add_epi32(*reinterpret_cast<__m128i*>(&writeValue), *reinterpret_cast<__m128i*>(&m_R));
				}
#endif
				else
				{
					for(unsigned int i = 0; i < 4; i++)
					{
						uint32 maskOp = useMask ? (colMask >> (i * 2)) & 0x3 : MASK_DATA;

						if(maskOp == MASK_DATA)
						{
							if(mode == MODE_OFFSET)
							{
								writeValue.nV[i] += m_R[i];
							}
							else if(mode == MODE_DIFFERENCE)
							{
								writeValue.nV[i] += m_R[i];
								m_R[i] = writeValue.nV[i];
							}

							dst->nV[i] = writeValue.nV[i];
						}
						else if(maskOp == MASK_ROW)
						{
							dst->nV[i] = m_R[i];
						}
						else if(maskOp == MASK_COL)
						{
							dst->nV[i] = m_C[col];
						}
						else if(maskOp == MASK_MASK)
						{
							//Don't write anything
						}
						else
						{
							assert(0);
						}
					}
				}

				currentNum--;
			}

			if(clGreaterEqualWl)
			{
				m_writeTick = std::min<uint32>(m_writeTick + 1, wl);
				m_readTick = std::min<uint32>(m_readTick + 1, cl);

				if(m_readTick == cl)
				{
					m_writeTick = 0;
					m_readTick = 0;
				}
			}
			else
			{
				m_writeTick = std::min<uint32>(m_writeTick + 1, wl);
				m_readTick = std::min<uint32>(m_readTick + 1, cl);

				if(m_writeTick == wl)
				{
					m_writeTick = 0;
					m_readTick = 0;
				}
			}

			nDstAddr += 0x10;
			nDstAddr &= (vuMemSize - 1);
		}

		if(currentNum != 0)
		{
			m_STAT.nVPS = 1;
		}
		else
		{
			stream.Align32();
			m_STAT.nVPS = 0;
		}

		m_NUM = static_cast<uint8>(currentNum);
	}

	typedef void (CVif::*Unpacker)(StreamType&, CODE, uint32);

	enum
	{
		MAX_UNPACKERS = 0x200
	};

	virtual void PrepareMicroProgram();
	void StartMicroProgram(uint32);
	void StartDelayedMicroProgram(uint32);
	bool ResumeDelayedMicroProgram();

	void DisassembleCommand(CODE);
	void DisassembleGet(uint32);
	void DisassembleSet(uint32, uint32);

	unsigned int m_number = 0;
	CVpu& m_vpu;
	CINTC& m_intc;
	uint8* m_ram = nullptr;
	uint8* m_spr = nullptr;
	CFifoStream m_stream;
	Unpacker m_unpacker[MAX_UNPACKERS];

	uint8 m_fifoBuffer[FIFO_SIZE];
	uint32 m_fifoIndex = 0;

	STAT m_STAT;
	ERR m_ERR;
	CYCLE m_CYCLE;
	CODE m_CODE;
	uint8 m_NUM;
	uint32 m_MODE;
	alignas(16) uint32 m_R[4];
	uint32 m_C[4];
	uint32 m_MASK;
	uint32 m_MARK;
	uint32 m_ITOP;
	uint32 m_ITOPS;
	uint32 m_readTick;
	uint32 m_writeTick;
	uint32 m_pendingMicroProgram;
	uint32 m_incomingFifoDelay;
	int32 m_interruptDelayTicks;
	bool m_delayDmaCompletion;
	uint32 m_delayedDmaQwc;

	CProfiler::ZoneHandle m_vifProfilerZone = 0;
};
