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

static int32 sdiv_stub(int32 op1, int32 op2)
{
	return op1 / op2;
}

static int32 smod_stub(int32 op1, int32 op2)
{
	return op1 % op2;
}

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
	m_assembler.Sub(CArmAssembler::rSP, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(0x04, 0));
	m_assembler.Str(CArmAssembler::rLR, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(0x00));
}

void CCodeGen::End()
{
	DumpLiteralPool();
}

void CCodeGen::EndQuota()
{
	m_assembler.Ldr(CArmAssembler::rLR, CArmAssembler::rSP, CArmAssembler::MakeImmediateLdrAddress(0x00));
	m_assembler.Add(CArmAssembler::rSP, CArmAssembler::rSP, CArmAssembler::MakeImmediateAluOperand(0x04, 0));
	m_assembler.Blx(CArmAssembler::rLR);
}

void CCodeGen::BeginIf(bool nCondition)
{
	if(FitsPattern<SingleRegister>())
	{
		SingleRegister::PatternValue op = GetPattern<SingleRegister>();
		CArmAssembler::REGISTER registerId = static_cast<CArmAssembler::REGISTER>(op);
		
		m_assembler.Teq(registerId, CArmAssembler::MakeImmediateAluOperand(0, 0));
		
		if(!RegisterHasNextUse(registerId))
		{
			FreeRegister(registerId);
		}
		
		CArmAssembler::LABEL ifLabel = m_assembler.CreateLabel();
		m_assembler.BCc(nCondition ? CArmAssembler::CONDITION_EQ : CArmAssembler::CONDITION_NE, ifLabel);
	
		m_IfStack.Push(ifLabel);
		m_IfStack.Push(IFBLOCK);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::EndIf()
{
	assert(m_IfStack.GetAt(0) == IFBLOCK);
	
	m_IfStack.Pull();
	CArmAssembler::LABEL ifLabel = static_cast<CArmAssembler::LABEL>(m_IfStack.Pull());
	
	m_assembler.MarkLabel(ifLabel);
	m_assembler.ResolveLabelReferences();
}

void CCodeGen::BeginIfElse(bool nCondition)
{
	if(FitsPattern<SingleRelative>())
	{
		SingleRelative::PatternValue op = GetPattern<SingleRelative>();

		CArmAssembler::REGISTER registerId = AllocateRegister();

		LoadRelativeInRegister(registerId, op);
		m_assembler.Teq(registerId, CArmAssembler::MakeImmediateAluOperand(0, 0));
		FreeRegister(registerId);
		
		CArmAssembler::LABEL ifLabel = m_assembler.CreateLabel();
		m_assembler.BCc(nCondition ? CArmAssembler::CONDITION_EQ : CArmAssembler::CONDITION_NE, ifLabel);
		
		m_IfStack.Push(ifLabel);
		m_IfStack.Push(IFELSEBLOCK);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::BeginIfElseAlt()
{
	assert(m_IfStack.GetAt(0) == IFELSEBLOCK);
	
	m_IfStack.Pull();
    CArmAssembler::LABEL ifLabel = m_IfStack.Pull();
	
    CArmAssembler::LABEL doneLabel = m_assembler.CreateLabel();
	
    //jmp label
    m_assembler.BCc(CArmAssembler::CONDITION_AL, doneLabel);
    m_assembler.MarkLabel(ifLabel);
	
	m_IfStack.Push(doneLabel);
	m_IfStack.Push(IFBLOCK);
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
	else if(FitsPattern<RelativeRelative>())
	{
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());
		
		CArmAssembler::REGISTER dstReg = AllocateRegister();
		CArmAssembler::REGISTER srcReg = AllocateRegister();
			
		LoadRelativeInRegister(dstReg, ops.first);
		LoadRelativeInRegister(srcReg, ops.second);
			
		//add reg, Immediate
		m_assembler.Add(dstReg, dstReg, srcReg);
			
		PushReg(dstReg);
		FreeRegister(srcReg);
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
	GenericCommutativeOperation(
								tr1::bind(&CArmAssembler::And, &m_assembler, _1, _2, _3),
								tr1::bind(&CCodeGen::Const_And, _1, _2)
	);
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
	if(FitsPattern<RegisterRegister>())
	{
        RegisterRegister::PatternValue ops(GetPattern<RegisterRegister>());
		
		CArmAssembler::REGISTER dstReg = static_cast<CArmAssembler::REGISTER>(ops.first);
		CArmAssembler::REGISTER srcReg = static_cast<CArmAssembler::REGISTER>(ops.second);
		CArmAssembler::REGISTER resultReg;
		
		if(!RegisterHasNextUse(dstReg))
		{
			resultReg = dstReg;
		}
		else
		{
			resultReg = AllocateRegister();
		}

		if(!RegisterHasNextUse(srcReg))
		{
			FreeRegister(srcReg);
		}
		
		CArmAssembler::CONDITION conditionCode = CArmAssembler::CONDITION_AL;
		
		switch(nCondition)
		{
			case CONDITION_LT:
				conditionCode = CArmAssembler::CONDITION_LT;
				break;
			case CONDITION_LE:
				conditionCode = CArmAssembler::CONDITION_LE;
				break;
			case CONDITION_EQ:
				conditionCode = CArmAssembler::CONDITION_EQ;
				break;
			case CONDITION_BL:
				conditionCode = CArmAssembler::CONDITION_CC;
				break;
			default:
				throw std::exception();
				break;
		}
		
		m_assembler.Cmp(dstReg, srcReg);
		m_assembler.Eor(resultReg, resultReg, resultReg);
		m_assembler.MovCc(conditionCode, resultReg, CArmAssembler::MakeImmediateAluOperand(1, 0));
		
		PushReg(resultReg);
	}
	else if(FitsPattern<RelativeConstant>())
	{
        RelativeConstant::PatternValue ops(GetPattern<RelativeConstant>());
		
		CArmAssembler::REGISTER dstReg = AllocateRegister();
		CArmAssembler::REGISTER srcReg = AllocateRegister();
			
		LoadRelativeInRegister(dstReg, ops.first);
		LoadConstantInRegister(srcReg, ops.second);

		PushReg(dstReg);
		PushReg(srcReg);
		
		Cmp(nCondition);
	}
	else if(FitsPattern<ConstantRelative>())
	{
        ConstantRelative::PatternValue ops(GetPattern<ConstantRelative>());
		
		CArmAssembler::REGISTER dstReg = AllocateRegister();
		CArmAssembler::REGISTER srcReg = AllocateRegister();
		
		LoadConstantInRegister(dstReg, ops.first);
		LoadRelativeInRegister(srcReg, ops.second);
		
		PushReg(dstReg);
		PushReg(srcReg);
		
		Cmp(nCondition);
	}
	else if(FitsPattern<RelativeRelative>())
	{
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());
		
		CArmAssembler::REGISTER dstReg = AllocateRegister();
		CArmAssembler::REGISTER srcReg = AllocateRegister();
		
		LoadRelativeInRegister(dstReg, ops.first);
		LoadRelativeInRegister(srcReg, ops.second);
		
		PushReg(dstReg);
		PushReg(srcReg);
		
		Cmp(nCondition);
	}
	else if(FitsPattern<ConstantRegister>())
	{
        ConstantRegister::PatternValue ops(GetPattern<ConstantRegister>());
		
		CArmAssembler::REGISTER dstReg = AllocateRegister();
		
		LoadConstantInRegister(dstReg, ops.first);
		
		PushReg(dstReg);
		PushReg(ops.second);
		
		Cmp(nCondition);
	}
	else
	{
		throw std::exception();
	}	
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
	if(FitsPattern<RelativeRelative>())
	{
		RelativeRelative::PatternValue ops = GetPattern<RelativeRelative>();
		void* divFunc = reinterpret_cast<void*>(sdiv_stub);
		void* modFunc = reinterpret_cast<void*>(smod_stub);

		CArmAssembler::REGISTER lowRegister = AllocateRegister();
		CArmAssembler::REGISTER highRegister = AllocateRegister();
		CArmAssembler::REGISTER tempRegister = AllocateRegister();

		LoadRelativeInRegister(lowRegister, ops.first);
		LoadRelativeInRegister(highRegister, ops.second);
		
		m_assembler.Mov(CArmAssembler::r0, lowRegister);
		m_assembler.Mov(CArmAssembler::r1, highRegister);
		LoadConstantInRegister(CArmAssembler::r2, reinterpret_cast<uint32>(divFunc));
		m_assembler.Blx(CArmAssembler::r2);
		m_assembler.Mov(tempRegister, CArmAssembler::r0);

		m_assembler.Mov(CArmAssembler::r0, lowRegister);
		m_assembler.Mov(CArmAssembler::r1, highRegister);
		LoadConstantInRegister(CArmAssembler::r2, reinterpret_cast<uint32>(modFunc));
		m_assembler.Blx(CArmAssembler::r2);

		m_assembler.Mov(lowRegister, tempRegister);
		m_assembler.Mov(highRegister, CArmAssembler::r0);
		
		FreeRegister(tempRegister);

		PushReg(highRegister);
		PushReg(lowRegister);
	}
	else
	{
		throw std::exception();
	}
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
	if(FitsPattern<RelativeRelative>())
	{
		RelativeRelative::PatternValue ops = GetPattern<RelativeRelative>();
		
		CArmAssembler::REGISTER lowRegister = AllocateRegister();
		CArmAssembler::REGISTER highRegister = AllocateRegister();

		LoadRelativeInRegister(lowRegister, ops.first);
		LoadRelativeInRegister(highRegister, ops.second);
		
		m_assembler.Smull(lowRegister, highRegister, lowRegister, highRegister);
		
		PushReg(highRegister);
		PushReg(lowRegister);
	}
	else if(FitsPattern<CommutativeRelativeConstant>())
	{
		CommutativeRelativeConstant::PatternValue ops = GetPattern<CommutativeRelativeConstant>();
		
		CArmAssembler::REGISTER lowRegister = AllocateRegister();
		CArmAssembler::REGISTER highRegister = AllocateRegister();
		
		LoadRelativeInRegister(lowRegister, ops.first);
		LoadConstantInRegister(highRegister, ops.second);
		
		m_assembler.Smull(lowRegister, highRegister, lowRegister, highRegister);
		
		PushReg(highRegister);
		PushReg(lowRegister);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::Not()
{
	if(FitsPattern<SingleRegister>())
	{
		SingleRegister::PatternValue op = GetPattern<SingleRegister>();
		
		CArmAssembler::REGISTER sourceRegister = static_cast<CArmAssembler::REGISTER>(op);
		CArmAssembler::REGISTER resultRegister;
		if(!RegisterHasNextUse(sourceRegister))
		{
			resultRegister = sourceRegister;
		}
		else
		{
			resultRegister = AllocateRegister();
		}
		
		m_assembler.Mvn(resultRegister, sourceRegister);
		
		PushReg(resultRegister);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::Or()
{
	GenericCommutativeOperation(
								tr1::bind(&CArmAssembler::Or, &m_assembler, _1, _2, _3),
								tr1::bind(&CCodeGen::Const_Or, _1, _2)
	);
}

void CCodeGen::SeX()
{
	throw std::exception();
}

void CCodeGen::SeX8()
{
	if(FitsPattern<SingleRegister>())
	{
		SingleRegister::PatternValue op = GetPattern<SingleRegister>();
		CArmAssembler::REGISTER srcRegister = static_cast<CArmAssembler::REGISTER>(op);
		CArmAssembler::REGISTER dstRegister;
		
		if(!RegisterHasNextUse(srcRegister))
		{
			dstRegister = srcRegister;
		}
		else
		{
			dstRegister = AllocateRegister();
		}
		
		m_assembler.Mov(dstRegister, 
						CArmAssembler::MakeRegisterAluOperand(srcRegister,
															  CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_LSL, 24)
															  )
						);
		m_assembler.Mov(dstRegister, 
						CArmAssembler::MakeRegisterAluOperand(dstRegister,
															  CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_ASR, 24)
															  )
						);
		
		PushReg(dstRegister);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::SeX16()
{
	if(FitsPattern<SingleRegister>())
	{
		SingleRegister::PatternValue op = GetPattern<SingleRegister>();
		CArmAssembler::REGISTER srcRegister = static_cast<CArmAssembler::REGISTER>(op);
		CArmAssembler::REGISTER dstRegister;
		
		if(!RegisterHasNextUse(srcRegister))
		{
			dstRegister = srcRegister;
		}
		else
		{
			dstRegister = AllocateRegister();
		}
		
		m_assembler.Mov(dstRegister, 
						CArmAssembler::MakeRegisterAluOperand(srcRegister,
															  CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_LSL, 16)
															  )
						);
		m_assembler.Mov(dstRegister, 
						CArmAssembler::MakeRegisterAluOperand(dstRegister,
															  CArmAssembler::MakeConstantShift(CArmAssembler::SHIFT_ASR, 16)
															  )
						);
		
		PushReg(dstRegister);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::Shl()
{
	GenericVariableShift(CArmAssembler::SHIFT_LSL);
}

void CCodeGen::Shl(uint8 nAmount)
{
	GenericConstantShift(CArmAssembler::SHIFT_LSL, nAmount);
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
	GenericConstantShift(CArmAssembler::SHIFT_ASR, nAmount);
}

void CCodeGen::Sra64(uint8 nAmount)
{
	throw std::exception();
}

void CCodeGen::Srl()
{
	GenericVariableShift(CArmAssembler::SHIFT_LSR);
}

void CCodeGen::Srl(uint8 nAmount)
{
	GenericConstantShift(CArmAssembler::SHIFT_LSR, nAmount);
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
	if(FitsPattern<ConstantRelative>())
	{
		ConstantRelative::PatternValue ops = GetPattern<ConstantRelative>();
		
		CArmAssembler::REGISTER dstRegister = AllocateRegister();
		CArmAssembler::REGISTER srcRegister = AllocateRegister();
		
		LoadConstantInRegister(dstRegister, ops.first);
		LoadRelativeInRegister(srcRegister, ops.second);
		
		m_assembler.Sub(dstRegister, dstRegister, srcRegister);
		
		FreeRegister(srcRegister);
		PushReg(dstRegister);
	}
	else if(FitsPattern<RelativeRelative>())
	{
		RelativeRelative::PatternValue ops = GetPattern<RelativeRelative>();
		
		CArmAssembler::REGISTER dstRegister = AllocateRegister();
		CArmAssembler::REGISTER srcRegister = AllocateRegister();
		
		LoadRelativeInRegister(dstRegister, ops.first);
		LoadRelativeInRegister(srcRegister, ops.second);
		
		m_assembler.Sub(dstRegister, dstRegister, srcRegister);
		
		FreeRegister(srcRegister);
		PushReg(dstRegister);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::Sub64()
{
	throw std::exception();
}

void CCodeGen::Xor()
{
	GenericCommutativeOperation(
								tr1::bind(&CArmAssembler::Eor, &m_assembler, _1, _2, _3),
								tr1::bind(&CCodeGen::Const_Xor, _1, _2)
								);
}

void CCodeGen::GenericCommutativeOperation(const TriRegOp& regOp, const ConstConstOp& constOp)
{
	if(FitsPattern<CommutativeRelativeConstant>())
	{
        CommutativeRelativeConstant::PatternValue ops(GetPattern<CommutativeRelativeConstant>());
		
		CArmAssembler::REGISTER dstReg = AllocateRegister();
		CArmAssembler::REGISTER srcReg = AllocateRegister();
		
		LoadRelativeInRegister(dstReg, ops.first);
		LoadConstantInRegister(srcReg, ops.second);
		
		regOp(dstReg, dstReg, srcReg);
		
		PushReg(dstReg);
		FreeRegister(srcReg);
	}
	else if(FitsPattern<RelativeRelative>())
	{
        RelativeRelative::PatternValue ops(GetPattern<RelativeRelative>());
		
		CArmAssembler::REGISTER dstReg = AllocateRegister();
		CArmAssembler::REGISTER srcReg = AllocateRegister();
		
		LoadRelativeInRegister(dstReg, ops.first);
		LoadRelativeInRegister(srcReg, ops.second);
		
		regOp(dstReg, dstReg, srcReg);
		
		PushReg(dstReg);
		FreeRegister(srcReg);
	}
	else if(FitsPattern<ConstantConstant>())
	{
		ConstantConstant::PatternValue ops(GetPattern<ConstantConstant>());
		uint32 result = constOp(ops.first, ops.second);
		PushCst(result);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::GenericVariableShift(CArmAssembler::SHIFT shiftType)
{
	if(FitsPattern<ConstantRelative>())
	{
		ConstantRelative::PatternValue ops = GetPattern<ConstantRelative>();
		
		CArmAssembler::REGISTER resultRegister = AllocateRegister();
		CArmAssembler::REGISTER shiftRegister = AllocateRegister();
		
		LoadConstantInRegister(resultRegister, ops.first);
		LoadRelativeInRegister(shiftRegister, ops.second);
		
		m_assembler.Mov(resultRegister, 
						CArmAssembler::MakeRegisterAluOperand(resultRegister,
															  CArmAssembler::MakeVariableShift(shiftType, shiftRegister)
															  )
						);
	
		FreeRegister(shiftRegister);
		PushReg(resultRegister);
	}
	else if(FitsPattern<RelativeRelative>())
	{
		RelativeRelative::PatternValue ops = GetPattern<RelativeRelative>();
		
		CArmAssembler::REGISTER resultRegister = AllocateRegister();
		CArmAssembler::REGISTER shiftRegister = AllocateRegister();
		
		LoadRelativeInRegister(resultRegister, ops.first);
		LoadRelativeInRegister(shiftRegister, ops.second);
		
		m_assembler.Mov(resultRegister, 
						CArmAssembler::MakeRegisterAluOperand(resultRegister,
															  CArmAssembler::MakeVariableShift(shiftType, shiftRegister)
															  )
						);
		
		FreeRegister(shiftRegister);
		PushReg(resultRegister);
	}
	else
	{
		throw std::exception();
	}
}

void CCodeGen::GenericConstantShift(CArmAssembler::SHIFT shiftType, uint8 nAmount)
{
	if(FitsPattern<SingleRelative>())
	{
		SingleRelative::PatternValue op = GetPattern<SingleRelative>();
		
		CArmAssembler::REGISTER resultReg = AllocateRegister();
		
		LoadRelativeInRegister(resultReg, op);
		
		m_assembler.Mov(resultReg, 
						CArmAssembler::MakeRegisterAluOperand(resultReg,
															  CArmAssembler::MakeConstantShift(shiftType, nAmount)
															  )
						);
		
		PushReg(resultReg);
	}
	else
	{
		throw std::exception();
	}
}

uint32 CCodeGen::Const_And(uint32 op1, uint32 op2)
{
	return op1 & op2;
}

uint32 CCodeGen::Const_Or(uint32 op1, uint32 op2)
{
	return op1 | op2;
}

uint32 CCodeGen::Const_Xor(uint32 op1, uint32 op2)
{
	return op1 ^ op2;
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
