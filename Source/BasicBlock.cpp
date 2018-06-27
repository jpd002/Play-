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

#pragma pack(push, 1)
struct AOT_BLOCK
{
	AOT_BLOCK_KEY key;
	void* fct;
};
#pragma pack(pop)

extern "C"
{
	extern AOT_BLOCK _aot_firstBlock;
	extern uint32 _aot_blockCount;
}

#endif

#define INVALID_LINK_SLOT (~0U)

CBasicBlock::CBasicBlock(CMIPS& context, uint32 begin, uint32 end)
    : m_begin(begin)
    , m_end(end)
    , m_context(context)
#ifdef AOT_USE_CACHE
    , m_function(nullptr)
#endif
{
	assert(m_end >= m_begin);
	for(uint32 i = 0; i < LINK_SLOT_MAX; i++)
	{
		m_linkBlockTrampolineOffset[i] = INVALID_LINK_SLOT;
	}
}

#ifdef AOT_BUILD_CACHE

Framework::CStdStream* CBasicBlock::m_aotBlockOutputStream(nullptr);
std::mutex CBasicBlock::m_aotBlockOutputStreamMutex;

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
		static
#ifdef AOT_BUILD_CACHE
		    __declspec(thread)
#endif
		        CMipsJitter* jitter = nullptr;
		if(jitter == nullptr)
		{
			Jitter::CCodeGen* codeGen = Jitter::CreateCodeGen();
			jitter = new CMipsJitter(codeGen);

			for(unsigned int i = 0; i < 4; i++)
			{
				jitter->SetVariableAsConstant(
				    offsetof(CMIPS, m_State.nGPR[CMIPS::R0].nV[i]),
				    0);
			}
		}

		jitter->GetCodeGen()->SetExternalSymbolReferencedHandler([&](auto symbol, auto offset) { HandleExternalFunctionReference(symbol, offset); });
		jitter->SetStream(&stream);
		jitter->Begin();
		CompileRange(jitter);
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

		auto functionName = string_format("BasicBlock_0x%08X_0x%08X", m_begin, m_end);

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

	AOT_BLOCK blockRef = {blockChecksum, m_begin, m_end, nullptr};

	static const auto blockComparer =
	    [](const AOT_BLOCK& item1, const AOT_BLOCK& item2) {
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
	if(IsEmpty())
	{
		jitter->JumpTo(reinterpret_cast<void*>(&EmptyBlockHandler));
		return;
	}

	for(uint32 address = m_begin; address <= m_end; address += 4)
	{
		m_context.m_pArch->CompileInstruction(
		    address,
		    jitter,
		    &m_context);
		//Sanity check
		assert(jitter->IsStackEmpty());
	}

	CompileProlog(jitter);
}

void CBasicBlock::CompileProlog(CMipsJitter* jitter)
{
	//Update cycle quota
	jitter->PushRel(offsetof(CMIPS, m_State.cycleQuota));
	jitter->PushCst(((m_end - m_begin) / 4) + 1);
	jitter->Sub();
	jitter->PullRel(offsetof(CMIPS, m_State.cycleQuota));

	jitter->PushRel(offsetof(CMIPS, m_State.cycleQuota));
	jitter->PushCst(0);
	jitter->BeginIf(Jitter::CONDITION_LE);
	{
		jitter->PushRel(offsetof(CMIPS, m_State.nHasException));
		jitter->PushCst(MIPS_EXECUTION_STATUS_QUOTADONE);
		jitter->Or();
		jitter->PullRel(offsetof(CMIPS, m_State.nHasException));
	}
	jitter->EndIf();

	//We probably don't need to pay for this since we know in advance if there's a branch
	jitter->PushCst(MIPS_INVALID_PC);
	jitter->PushRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
	jitter->BeginIf(Jitter::CONDITION_NE);
	{
		jitter->PushRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
		jitter->PullRel(offsetof(CMIPS, m_State.nPC));

		jitter->PushCst(MIPS_INVALID_PC);
		jitter->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

		jitter->PushRel(offsetof(CMIPS, m_State.nHasException));
		jitter->PushCst(0);
		jitter->BeginIf(Jitter::CONDITION_EQ);
		{
			jitter->JumpTo(reinterpret_cast<void*>(&NextBlockTrampoline));
		}
		jitter->EndIf();
	}
	jitter->Else();
	{
		jitter->PushCst(m_end + 4);
		jitter->PullRel(offsetof(CMIPS, m_State.nPC));

		jitter->PushRel(offsetof(CMIPS, m_State.nHasException));
		jitter->PushCst(0);
		jitter->BeginIf(Jitter::CONDITION_EQ);
		{
			jitter->JumpTo(reinterpret_cast<void*>(&NextBlockTrampoline));
		}
		jitter->EndIf();
	}
	jitter->EndIf();
}

void CBasicBlock::Execute()
{
	m_function(&m_context);

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

bool CBasicBlock::IsEmpty() const
{
	return (m_begin == MIPS_INVALID_PC) &&
	       (m_end == MIPS_INVALID_PC);
}

void CBasicBlock::LinkBlock(LINK_SLOT linkSlot, CBasicBlock* otherBlock)
{
	assert(!IsEmpty());
	assert(!otherBlock->IsEmpty());
	assert(linkSlot < LINK_SLOT_MAX);
	assert(m_linkBlockTrampolineOffset[linkSlot] != INVALID_LINK_SLOT);
	auto patchValue = reinterpret_cast<uintptr_t>(otherBlock->m_function.GetCode());
	auto code = reinterpret_cast<uint8*>(m_function.GetCode());
	*reinterpret_cast<uintptr_t*>(code + m_linkBlockTrampolineOffset[linkSlot]) = patchValue;
}

void CBasicBlock::HandleExternalFunctionReference(uintptr_t symbol, uint32 offset)
{
	if(symbol == reinterpret_cast<uintptr_t>(&NextBlockTrampoline))
	{
		if(m_linkBlockTrampolineOffset[LINK_SLOT_BRANCH] == INVALID_LINK_SLOT)
		{
			m_linkBlockTrampolineOffset[LINK_SLOT_BRANCH] = offset;
		}
		else
		{
			m_linkBlockTrampolineOffset[LINK_SLOT_NEXT] = offset;
		}
	}
}

void CBasicBlock::EmptyBlockHandler(CMIPS* context)
{
	context->m_emptyBlockHandler(context);
}

void CBasicBlock::NextBlockTrampoline(CMIPS* context)
{

}
