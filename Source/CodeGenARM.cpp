#include <assert.h>
#include "CodeGenARM.h"
#include "PtrMacro.h"
#include "CodeGen_StackPatterns.h"
#include "placeholder_def.h"

using namespace Framework;
using namespace std;

CArmAssembler::REGISTER CCodeGen::g_baseRegister = CArmAssembler::r11;

CArmAssembler::REGISTER CCodeGen::g_usableRegisters[MAX_USABLE_REGISTERS] =
{
//	CArmAssembler::r0,
//	CArmAssembler::r1,
//	CArmAssembler::r2,
//	CArmAssembler::r3,
	CArmAssembler::r4,
	CArmAssembler::r5,
	CArmAssembler::r6,
	CArmAssembler::r7,
	CArmAssembler::r8,
	CArmAssembler::r10,
};

CCodeGen::CCodeGen() :
m_assembler(
		tr1::bind(&CCodeGen::StreamWriteByte, this, PLACEHOLDER_1),
        tr1::bind(&CCodeGen::StreamWriteAt, this, PLACEHOLDER_1, PLACEHOLDER_2),
        tr1::bind(&CCodeGen::StreamTell, this)
        ),
m_literalPool(NULL)
{
	for(unsigned int i = 0; i < MAX_USABLE_REGISTERS; i++)
	{
		m_registerAllocated[i] = false;
	}
	m_literalPool = new uint32[LITERAL_POOL_SIZE];
}

CCodeGen::~CCodeGen()
{
	if(m_literalPool)
	{
		delete [] m_literalPool;
		m_literalPool = NULL;
	}
}

bool CCodeGen::AreAllRegistersFreed() const
{
	for(unsigned int i = 0; i < MAX_USABLE_REGISTERS; i++)
	{
		if(m_registerAllocated[i]) return false;
	}
	
	return true;
}

void CCodeGen::Begin()
{
	CCodeGenBase::Begin();
	for(unsigned int i = 0; i < MAX_USABLE_REGISTERS; i++)
	{
		m_registerAllocated[i] = false;
	}
	m_lastLiteralPtr = 0;
	
	//Save return address
	m_assembler.Sub(CArmAssembler::rSP, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(4, 0));
	m_assembler.Str(CArmAssembler::rLR, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(0));
}

void CCodeGen::End()
{
	DumpLiteralPool();
}

void CCodeGen::EndQuota()
{
	m_assembler.Ldr(CArmAssembler::rLR, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(0));
	m_assembler.Add(CArmAssembler::rSP, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(4, 0));
	m_assembler.Blx(CArmAssembler::rLR);
}

void CCodeGen::BeginIf(bool nCondition)
{
	throw std::exception();
}

void CCodeGen::EndIf()
{
	throw std::exception();
}

void CCodeGen::BeginIfElse(bool nCondition)
{
	throw std::exception();
}

void CCodeGen::BeginIfElseAlt()
{
	throw std::exception();
}

void CCodeGen::PushReg(unsigned int registerId)
{
	m_Shadow.Push(registerId);
	m_Shadow.Push(REGISTER);	
}

void CCodeGen::PushReg(CArmAssembler::REGISTER registerId)
{
	PushReg(static_cast<unsigned int>(registerId));
}

void CCodeGen::PullRel(size_t nOffset)
{
	if(FitsPattern<SingleRegister>())
	{
		SingleRegister::PatternValue op(GetPattern<SingleRegister>());
		CArmAssembler::REGISTER dstRegister = static_cast<CArmAssembler::REGISTER>(op);
		m_assembler.Str(dstRegister, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(nOffset));
		
		if(!RegisterHasNextUse(dstRegister))
		{
			FreeRegister(dstRegister);			
		}
	}
	else if(FitsPattern<SingleRelative>())
	{
		SingleRelative::PatternValue op(GetPattern<SingleRelative>());

		CArmAssembler::REGISTER tmpRegister = AllocateRegister();
		LoadRelativeInRegister(tmpRegister, op);
		m_assembler.Str(tmpRegister, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(nOffset));
		FreeRegister(tmpRegister);
	}
	else if(FitsPattern<SingleConstant>())
	{
		SingleConstant::PatternValue op(GetPattern<SingleConstant>());
		
		CArmAssembler::REGISTER tmpRegister = AllocateRegister();
		LoadConstantInRegister(tmpRegister, op);
		m_assembler.Str(tmpRegister, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(nOffset));
		FreeRegister(tmpRegister);
	}
	else
	{
		throw std::exception();		
	}
}

void CCodeGen::PullTop()
{
    uint32 type = m_Shadow.Pull();
    uint32 value = m_Shadow.Pull();
    if(type == REGISTER)
    {
		CArmAssembler::REGISTER registerId = static_cast<CArmAssembler::REGISTER>(value);
        if(!RegisterHasNextUse(registerId))
        {
            FreeRegister(registerId);
        }
    }
}

void CCodeGen::Add()
{
	if(FitsPattern<CommutativeRelativeConstant>())
	{
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());
		
		if(ops.second == 0)
		{
			PushRel(ops.first);
		}
		else
		{
			CArmAssembler::REGISTER dstReg = AllocateRegister();
			CArmAssembler::REGISTER srcReg = AllocateRegister();

			LoadRelativeInRegister(dstReg, ops.first);
			LoadConstantInRegister(srcReg, ops.second);
			
			//add reg, Immediate
			m_assembler.Add(dstReg, dstReg, srcReg);
			
			PushReg(dstReg);
			FreeRegister(srcReg);
		}
	}
	else if(FitsPattern<ConstantConstant>())
	{
		ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
		PushCst(ops.first + ops.second);
	}
	else
	{
		throw std::exception();
	}	
}

void CCodeGen::Add64()
{
	throw std::exception();
}

void CCodeGen::And()
{
	throw std::exception();
}

void CCodeGen::And64()
{
	throw std::exception();
}

void CCodeGen::Call(void* pFunc, unsigned int nParamCount, bool nKeepRet)
{
	CArmAssembler::REGISTER funcPtr = AllocateRegister();
	
	assert(nParamCount <= 4);
	CArmAssembler::REGISTER argumentReg = static_cast<CArmAssembler::REGISTER>(CArmAssembler::r0 + nParamCount);
	
	for(unsigned int i = 0; i < nParamCount; i++)
	{
		uint32 type = m_Shadow.Pull();
		uint32 value = m_Shadow.Pull();
		
		argumentReg = static_cast<CArmAssembler::REGISTER>(argumentReg - 1);
	
		switch(type)
		{
		case CONSTANT:
		case REFERENCE:
			LoadConstantInRegister(argumentReg, value);
			break;
		case REGISTER:
			{
				CArmAssembler::REGISTER registerId = static_cast<CArmAssembler::REGISTER>(value);
				m_assembler.Mov(argumentReg, registerId);
				if(!RegisterHasNextUse(registerId))
				{
					FreeRegister(registerId);
				}
			}
			break;
		case RELATIVE:
			LoadRelativeInRegister(argumentReg, value);
			break;
		default:
			assert(0);
			break;
		}
	}

	LoadConstantInRegister(funcPtr, reinterpret_cast<uint32>(pFunc));
	m_assembler.Blx(funcPtr);
	FreeRegister(funcPtr);
	
	if(nKeepRet)
	{
		CArmAssembler::REGISTER resultRegister = AllocateRegister();
		m_assembler.Mov(resultRegister, CArmAssembler::r0);
		PushReg(resultRegister);
	}
}

void CCodeGen::Cmp(CONDITION nCondition)
{
	throw std::exception();
}

void CCodeGen::Cmp64(CONDITION nCondition)
{
	throw std::exception();
}

void CCodeGen::Div()
{
	throw std::exception();
}

void CCodeGen::DivS()
{
	throw std::exception();
}

void CCodeGen::Lookup(uint32* table)
{
	throw std::exception();
}

void CCodeGen::Lzc()
{
	throw std::exception();
}

void CCodeGen::Mult()
{
	throw std::exception();
}

void CCodeGen::MultS()
{
	throw std::exception();
}

void CCodeGen::Not()
{
	throw std::exception();
}

void CCodeGen::Or()
{
	throw std::exception();
}

void CCodeGen::SeX()
{
	throw std::exception();
}

void CCodeGen::SeX8()
{
	throw std::exception();
}

void CCodeGen::SeX16()
{
	throw std::exception();
}

void CCodeGen::Shl()
{
	throw std::exception();
}

void CCodeGen::Shl(uint8 nAmount)
{
	throw std::exception();
}

void CCodeGen::Shl64()
{
	throw std::exception();
}

void CCodeGen::Shl64(uint8 nAmount)
{
	throw std::exception();
}

void CCodeGen::Sra()
{
	throw std::exception();
}

void CCodeGen::Sra(uint8 nAmount)
{
	throw std::exception();
}

void CCodeGen::Sra64(uint8 nAmount)
{
	throw std::exception();
}

void CCodeGen::Srl()
{
	throw std::exception();
}

void CCodeGen::Srl(uint8 nAmount)
{
	throw std::exception();
}

void CCodeGen::Srl64()
{
	throw std::exception();
}

void CCodeGen::Srl64(uint8 nAmount)
{
	throw std::exception();
}

void CCodeGen::Sub()
{
	throw std::exception();
}

void CCodeGen::Sub64()
{
	throw std::exception();
}

void CCodeGen::Xor()
{
	throw std::exception();
}

CArmAssembler::REGISTER CCodeGen::AllocateRegister()
{
	for(unsigned int i = 0; i < MAX_USABLE_REGISTERS; i++)
	{
		if(!m_registerAllocated[i])
		{
			m_registerAllocated[i] = true;
			return g_usableRegisters[i];
		}
	}
	throw std::runtime_error("No register available for allocation.");
}

void CCodeGen::FreeRegister(CArmAssembler::REGISTER registerId)
{
	for(unsigned int i = 0; i < MAX_USABLE_REGISTERS; i++)
	{
		if(g_usableRegisters[i] == registerId)
		{
			m_registerAllocated[i] = false;
			return;
		}
	}
	throw std::runtime_error("Register not usable by compiler.");
}

bool CCodeGen::RegisterHasNextUse(CArmAssembler::REGISTER registerId)
{
	unsigned int nCount = m_Shadow.GetCount();
	for(unsigned int i = 0; i < nCount; i += 2)
	{
		if(m_Shadow.GetAt(i) == REGISTER)
		{
			if(m_Shadow.GetAt(i + 1) == registerId) return true;
		}
	}
	
	return false;
}

void CCodeGen::LoadRelativeInRegister(CArmAssembler::REGISTER registerId, uint32 relative)
{
	m_assembler.Ldr(registerId, g_baseRegister, CArmAssembler::MakeImmediateLdrAddress(relative));
}

void CCodeGen::LoadConstantInRegister(CArmAssembler::REGISTER registerId, uint32 constant)
{
	//Search for an existing literal
	unsigned int literalPtr = -1;
	for(unsigned int i = 0; i < m_lastLiteralPtr; i++)
	{
		if(m_literalPool[i] == constant) 
		{
			literalPtr = i;
			break;
		}
	}
	if(literalPtr == -1)
	{
		assert(m_lastLiteralPtr != LITERAL_POOL_SIZE);
		literalPtr = m_lastLiteralPtr++;
		m_literalPool[literalPtr] = constant;
	}
	
	LITERAL_POOL_REF reference;
	reference.poolPtr = literalPtr;
	reference.dstRegister = registerId;
	reference.offset = m_stream->Tell();
	m_literalPoolRefs.push_back(reference);
	
	//Write a blank instruction
	m_stream->Write32(0);
}

void CCodeGen::DumpLiteralPool()
{
	if(m_lastLiteralPtr)
	{
		uint32 literalPoolPos = m_stream->Tell();
		m_stream->Write(m_literalPool, sizeof(uint32) * m_lastLiteralPtr);
		
		for(LiteralPoolRefList::const_iterator referenceIterator(m_literalPoolRefs.begin());
			referenceIterator != m_literalPoolRefs.end(); referenceIterator++)
		{
			const LITERAL_POOL_REF& reference(*referenceIterator);
			m_stream->Seek(reference.offset, Framework::STREAM_SEEK_SET);
			uint32 literalOffset = (reference.poolPtr * 4 + literalPoolPos) - reference.offset - 8;
			m_assembler.Ldr(reference.dstRegister, CArmAssembler::rPC, CArmAssembler::MakeImmediateLdrAddress(literalOffset));
		}
	}
	
	m_literalPoolRefs.clear();
	m_lastLiteralPtr = 0;
}
