#include "BasicBlock.h"
#include "MemStream.h"
#include "offsetof_def.h"
#include "MipsJitter.h"
#include "Jitter_CodeGenFactory.h"

#if defined(AOT_BUILD_CACHE) || defined(AOT_USE_CACHE)
#define AOT_ENABLED
#endif

#ifdef AOT_ENABLED

#include <zlib.h>
#include "StdStream.h"
#include "StdStreamUtils.h"

#endif

#ifdef VTUNE_ENABLED
#include <jitprofiling.h>
#include "string_format.h"
#endif

#ifdef AOT_USE_CACHE

struct AOT_BLOCK
{
	AOT_BLOCK_KEY	key;
	void*			fct;
};

extern "C" AOT_BLOCK	_aot_firstBlock;
extern "C" uint32		_aot_blockCount;

#endif

CBasicBlock::CBasicBlock(CMIPS& context, uint32 begin, uint32 end)
: m_begin(begin)
, m_end(end)
, m_context(context)
, m_selfLoopCount(0)
#ifdef AOT_USE_CACHE
, m_function(nullptr)
#endif
{
	assert(m_end >= m_begin);
}

CBasicBlock::~CBasicBlock()
{

}

#ifdef AOT_BUILD_CACHE

Framework::CStdStream*	CBasicBlock::m_aotBlockOutputStream(nullptr);
std::mutex				CBasicBlock::m_aotBlockOutputStreamMutex;

void CBasicBlock::SetAotBlockOutputStream(Framework::CStdStream* outputStream)
{
	assert(m_aotBlockOutputStream == nullptr || outputStream == nullptr);
	m_aotBlockOutputStream = outputStream;
}

#endif

void CBasicBlock::Compile()
{
#ifndef AOT_USE_CACHE

	Framework::CMemStream stream;
	{
		static __declspec(thread) CMipsJitter* jitter = NULL;
		if(jitter == NULL)
		{
			Jitter::CCodeGen* codeGen = Jitter::CreateCodeGen();
			jitter = new CMipsJitter(codeGen);

			for(unsigned int i = 0; i < 4; i++)
			{
				jitter->SetVariableAsConstant(
					offsetof(CMIPS, m_State.nGPR[CMIPS::R0].nV[i]),
					0
					);
			}
		}

		jitter->SetStream(&stream);
		jitter->Begin();
		CompileRange(jitter);
//		codeGen.DumpVariables(0);
//		codeGen.EndQuota();
		jitter->End();
	}

	m_function = CMemoryFunction(stream.GetBuffer(), stream.GetSize());
	
#ifdef VTUNE_ENABLED
	if(iJIT_IsProfilingActive() == iJIT_SAMPLING_ON)
	{
		iJIT_Method_Load jmethod = {};
		jmethod.method_id = iJIT_GetNewMethodID();
		jmethod.class_file_name = "";
		jmethod.source_file_name = __FILE__;

		jmethod.method_load_address = m_function.GetCode();
		jmethod.method_size = m_function.GetSize();
		jmethod.line_number_size = 0;

		auto functionName = string_format("BasicBlock_0x%0.8X_0x%0.8X", m_begin, m_end);

		jmethod.method_name = const_cast<char*>(functionName.c_str());
		iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, reinterpret_cast<void*>(&jmethod));
	}
#endif

#endif

#ifdef AOT_ENABLED

	size_t blockSize = ((m_end - m_begin) / 4) + 1;
	auto blockData = new uint32[blockSize];

	for(uint32 i = 0; i < blockSize; i++)
	{
		blockData[i] = m_context.m_pMemoryMap->GetWord(m_begin + (i * 4));
	}

	uint32 blockChecksum = crc32(0, reinterpret_cast<Bytef*>(blockData), blockSize * 4);

#endif

#ifdef AOT_USE_CACHE

	AOT_BLOCK* blocksBegin = &_aot_firstBlock;
	AOT_BLOCK* blocksEnd = blocksBegin + _aot_blockCount;

	AOT_BLOCK blockRef = { blockChecksum, m_begin, m_end, nullptr };

	static const auto blockComparer = 
		[] (const AOT_BLOCK& item1, const AOT_BLOCK& item2)
		{
			return item1.key < item2.key;
		};

//	assert(std::is_sorted(blocksBegin, blocksEnd, blockComparer));

	bool blockExists = std::binary_search(blocksBegin, blocksEnd, blockRef, blockComparer);
	auto blockIterator = std::lower_bound(blocksBegin, blocksEnd, blockRef, blockComparer);
	
	assert(blockExists);
	assert(blockIterator != blocksEnd);
	assert(blockIterator->key.crc == blockChecksum);
	assert(blockIterator->key.begin == m_begin);
	assert(blockIterator->key.end == m_end);

	m_function = reinterpret_cast<void (*)(void*)>(blockIterator->fct);

#endif

#ifdef AOT_BUILD_CACHE
	{
		std::lock_guard<std::mutex> lock(m_aotBlockOutputStreamMutex);

		m_aotBlockOutputStream->Write32(blockChecksum);
		m_aotBlockOutputStream->Write32(m_begin);
		m_aotBlockOutputStream->Write32(m_end);
		m_aotBlockOutputStream->Write(blockData, blockSize * 4);
	}
#endif
}

void CBasicBlock::CompileRange(CMipsJitter* jitter)
{
	for(uint32 address = m_begin; address <= m_end; address += 4)
	{
		m_context.m_pArch->CompileInstruction(
			address, 
			jitter,
			&m_context);
		//Sanity check
		assert(jitter->IsStackEmpty());
	}
}

unsigned int CBasicBlock::Execute()
{
	m_function(&m_context);

	if(m_context.m_State.nDelayedJumpAddr != MIPS_INVALID_PC)
	{
		m_context.m_State.nPC = m_context.m_State.nDelayedJumpAddr;
		m_context.m_State.nDelayedJumpAddr = MIPS_INVALID_PC;
	}
	else
	{
		m_context.m_State.nPC = m_end + 4;
	}

	assert(m_context.m_State.nGPR[0].nV0 == 0);
	assert(m_context.m_State.nGPR[0].nV1 == 0);
	assert(m_context.m_State.nGPR[0].nV2 == 0);
	assert(m_context.m_State.nGPR[0].nV3 == 0);
	assert((m_context.m_State.nPC & 3) == 0);
	assert((m_context.m_State.nGPR[CMIPS::RA].nV0 & 3) == 0);
	assert(m_context.m_State.nCOP2[0].nV0 == 0x00000000);
	assert(m_context.m_State.nCOP2[0].nV1 == 0x00000000);
	assert(m_context.m_State.nCOP2[0].nV2 == 0x00000000);
	assert(m_context.m_State.nCOP2[0].nV3 == 0x3F800000);
	assert(m_context.m_State.nCOP2VI[0] == 0);

	return ((m_end - m_begin) / 4) + 1;
}

uint32 CBasicBlock::GetBeginAddress() const
{
	return m_begin;
}

uint32 CBasicBlock::GetEndAddress() const
{
	return m_end;
}

bool CBasicBlock::IsCompiled() const
{
#ifndef AOT_USE_CACHE
	return !m_function.IsEmpty();
#else
	return (m_function != nullptr);
#endif
}

unsigned int CBasicBlock::GetSelfLoopCount() const
{
	return m_selfLoopCount;
}

void CBasicBlock::SetSelfLoopCount(unsigned int selfLoopCount)
{
	m_selfLoopCount = selfLoopCount;
}
