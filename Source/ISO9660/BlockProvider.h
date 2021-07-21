#pragma once

#include <memory>
#include <cassert>
#include "Types.h"
#include "Stream.h"

namespace ISO9660
{
	class CBlockProvider
	{
	public:
		enum BLOCKSIZE
		{
			BLOCKSIZE = 0x800ULL
		};

		virtual ~CBlockProvider() = default;
		virtual void ReadBlock(uint32, void*) = 0;
		virtual void ReadRawBlock(uint32, void*) = 0;
		virtual uint32 GetBlockCount() = 0;
		virtual uint32 GetRawBlockSize() const = 0;
	};

	class CBlockProvider2048 : public CBlockProvider
	{
	public:
		typedef std::shared_ptr<Framework::CStream> StreamPtr;

		CBlockProvider2048(const StreamPtr& stream, uint32 offset = 0)
		    : m_stream(stream)
		    , m_offset(offset)
		{
		}

		void ReadBlock(uint32 address, void* block) override
		{
			m_stream->Seek(static_cast<uint64>(address + m_offset) * BLOCKSIZE, Framework::STREAM_SEEK_SET);
			m_stream->Read(block, BLOCKSIZE);
		}

		void ReadRawBlock(uint32 address, void* block) override
		{
			ReadBlock(address, block);
		}

		uint32 GetBlockCount() override
		{
			uint64 imageSize = m_stream->GetLength();
			assert((imageSize % BLOCKSIZE) == 0);
			return static_cast<uint32>(imageSize / BLOCKSIZE);
		}

		uint32 GetRawBlockSize() const override
		{
			return BLOCKSIZE;
		}

	private:
		StreamPtr m_stream;
		uint32 m_offset = 0;
	};

	template <uint64 INTERNAL_BLOCKSIZE, uint64 BLOCKHEADER_SIZE>
	class CBlockProviderCustom : public CBlockProvider
	{
	public:
		typedef std::shared_ptr<Framework::CStream> StreamPtr;

		CBlockProviderCustom(const StreamPtr& stream)
		    : m_stream(stream)
		{
		}

		void ReadBlock(uint32 address, void* block) override
		{
			m_stream->Seek((static_cast<uint64>(address) * INTERNAL_BLOCKSIZE) + BLOCKHEADER_SIZE, Framework::STREAM_SEEK_SET);
			m_stream->Read(block, BLOCKSIZE);
		}

		void ReadRawBlock(uint32 address, void* block) override
		{
			m_stream->Seek(static_cast<uint64>(address) * INTERNAL_BLOCKSIZE, Framework::STREAM_SEEK_SET);
			m_stream->Read(block, INTERNAL_BLOCKSIZE);
		}

		uint32 GetBlockCount() override
		{
			uint64 imageSize = m_stream->GetLength();
			assert((imageSize % INTERNAL_BLOCKSIZE) == 0);
			return static_cast<uint32>(imageSize / INTERNAL_BLOCKSIZE);
		}

		uint32 GetRawBlockSize() const override
		{
			return INTERNAL_BLOCKSIZE;
		}

	private:
		StreamPtr m_stream;
	};

	typedef CBlockProviderCustom<0x930ULL, 0x18ULL> CBlockProviderCDROMXA;
}
