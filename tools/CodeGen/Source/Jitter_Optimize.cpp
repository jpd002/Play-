#include <assert.h>
#include <set>
#include "Jitter.h"

#ifdef _DEBUG
#define DUMP_STATEMENTS
#endif

#ifdef DUMP_STATEMENTS
#include <iostream>
#endif

using namespace std;
using namespace Jitter;

static bool IsPowerOfTwo(uint32 number)
{
	uint32 complement = number - 1;
	return (number != 0) && ((number & complement) == 0);
}

static uint32 ones32(uint32 x)
{
	/* 32-bit recursive reduction using SWAR...
	   but first step is mapping 2-bit values
	   into sum of 2 1-bit values in sneaky way
	*/
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	return (x & 0x0000003f);
}

static uint32 GetPowerOf2(uint32 x)
{
	assert(IsPowerOfTwo(x));

	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);

	return ones32(x >> 1);
}

unsigned int CJitter::CRelativeVersionManager::GetRelativeVersion(uint32 relativeId)
{
	RelativeVersionMap::const_iterator versionIterator(m_relativeVersions.find(relativeId));
	if(versionIterator == m_relativeVersions.end()) return 0;
	return versionIterator->second;
}

unsigned int CJitter::CRelativeVersionManager::IncrementRelativeVersion(uint32 relativeId)
{
	unsigned int nextVersion = GetRelativeVersion(relativeId) + 1;
	m_relativeVersions[relativeId] = nextVersion;
	return nextVersion;
}

CJitter::VERSIONED_STATEMENT_LIST CJitter::GenerateVersionedStatementList(const StatementList& statements)
{
	VERSIONED_STATEMENT_LIST result;

	for(StatementList::const_iterator statementIterator(statements.begin());
		statements.end() != statementIterator; statementIterator++)
	{
		STATEMENT newStatement(*statementIterator);

		if(CSymbol* src1 = dynamic_symbolref_cast(SYM_RELATIVE, newStatement.src1))
		{
			unsigned int currentVersion = result.relativeVersions.GetRelativeVersion(src1->m_valueLow);
			newStatement.src1 = SymbolRefPtr(new CVersionedSymbolRef(newStatement.src1->GetSymbol(), currentVersion));
		}

		if(CSymbol* src2 = dynamic_symbolref_cast(SYM_RELATIVE, newStatement.src2))
		{
			unsigned int currentVersion = result.relativeVersions.GetRelativeVersion(src2->m_valueLow);
			newStatement.src2 = SymbolRefPtr(new CVersionedSymbolRef(newStatement.src2->GetSymbol(), currentVersion));
		}

		if(CSymbol* dst = dynamic_symbolref_cast(SYM_RELATIVE, newStatement.dst))
		{
			unsigned int nextVersion = result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow);
			newStatement.dst = SymbolRefPtr(new CVersionedSymbolRef(newStatement.dst->GetSymbol(), nextVersion));
		}

		if(CSymbol* dst = dynamic_symbolref_cast(SYM_RELATIVE64, newStatement.dst))
		{
			//Increment both relative versions to prevent some optimization problems
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 0);
			result.relativeVersions.IncrementRelativeVersion(dst->m_valueLow + 4);
		}

		result.statements.push_back(newStatement);
	}

	return result;
}

StatementList CJitter::CollapseVersionedStatementList(const VERSIONED_STATEMENT_LIST& statements)
{
	StatementList result;
	for(StatementList::const_iterator statementIterator(statements.statements.begin());
		statementIterator != statements.statements.end(); statementIterator++)
	{
		STATEMENT newStatement(*statementIterator);

		if(VersionedSymbolRefPtr src1 = std::tr1::dynamic_pointer_cast<CVersionedSymbolRef>(newStatement.src1))
		{
			newStatement.src1 = SymbolRefPtr(new CSymbolRef(src1->GetSymbol()));
		}

		if(VersionedSymbolRefPtr src2 = std::tr1::dynamic_pointer_cast<CVersionedSymbolRef>(newStatement.src2))
		{
			newStatement.src2 = SymbolRefPtr(new CSymbolRef(src2->GetSymbol()));
		}

		if(VersionedSymbolRefPtr dst = std::tr1::dynamic_pointer_cast<CVersionedSymbolRef>(newStatement.dst))
		{
			newStatement.dst = SymbolRefPtr(new CSymbolRef(dst->GetSymbol()));
		}

		result.push_back(newStatement);
	}
	return result;
}

void CJitter::Compile()
{
	while(1)
	{
		for(BasicBlockList::iterator blockIterator(m_basicBlocks.begin());
			m_basicBlocks.end() != blockIterator; blockIterator++)
		{
			BASIC_BLOCK& basicBlock(blockIterator->second);
			if(!basicBlock.optimized)
			{
				m_currentBlock = &basicBlock;

				VERSIONED_STATEMENT_LIST versionedStatements = GenerateVersionedStatementList(basicBlock.statements);

				while(1)
				{
					bool dirty = false;
					dirty |= ConstantPropagation(versionedStatements.statements);
					dirty |= ConstantFolding(versionedStatements.statements);
					dirty |= CopyPropagation(versionedStatements.statements);
					dirty |= DeadcodeElimination(versionedStatements);

					if(!dirty) break;
				}

				basicBlock.statements = CollapseVersionedStatementList(versionedStatements);
				basicBlock.optimized = true;
			}
		}

		bool dirty = false;
		dirty |= PruneBlocks();
		dirty |= MergeBlocks();

		if(!dirty) break;
	}

	//Allocate registers
	for(BasicBlockList::iterator blockIterator(m_basicBlocks.begin());
		m_basicBlocks.end() != blockIterator; blockIterator++)
	{
		BASIC_BLOCK& basicBlock(blockIterator->second);
		m_currentBlock = &basicBlock;

		CoalesceTemporaries(basicBlock);
		ComputeLivenessAndPruneSymbols(basicBlock);
		AllocateRegisters(basicBlock);
	}

	BASIC_BLOCK result = ConcatBlocks(m_basicBlocks);

#ifdef DUMP_STATEMENTS
	DumpStatementList(result.statements);
	cout << endl;
#endif

	unsigned int stackSize = AllocateStack(result);
	m_codeGen->GenerateCode(result.statements, stackSize);
}

void CJitter::DumpStatementList(const StatementList& statements)
{
#ifdef DUMP_STATEMENTS
	for(StatementList::const_iterator statementIterator(statements.begin());
		statements.end() != statementIterator; statementIterator++)
	{
		const STATEMENT& statement(*statementIterator);

		if(statement.dst)
		{
			cout << statement.dst->ToString();
			cout << " := ";
		}

		if(statement.src1)
		{
			cout << statement.src1->ToString();
		}

		switch(statement.op)
		{
		case OP_ADD:
		case OP_ADD64:
		case OP_FP_ADD:
			cout << " + ";
			break;
		case OP_SUB:
			cout << " - ";
			break;
		case OP_MUL:
		case OP_MULS:
			cout << " * ";
			break;
		case OP_DIV:
		case OP_FP_DIV:
			cout << " / ";
			break;
		case OP_AND:
			cout << " & ";
			break;
		case OP_OR:
			cout << " | ";
			break;
		case OP_XOR:
			cout << " ^ ";
			break;
		case OP_NOT:
			cout << " ! ";
			break;
		case OP_SRL:
			cout << " >> ";
			break;
		case OP_SRA:
			cout << " >>A ";
			break;
		case OP_SLL:
			cout << " << ";
			break;
		case OP_NOP:
			cout << " NOP ";
			break;
		case OP_MOV:
			break;
		case OP_PARAM:
			cout << " PARAM ";
			break;
		case OP_CALL:
			cout << " CALL ";
			break;
		case OP_RETVAL:
			cout << " RETURNVALUE ";
			break;
		case OP_JMP:
			cout << " JMP(" << statement.jmpBlock << ") ";
			break;
		case OP_CONDJMP:
			cout << " JMP(" << statement.jmpBlock << ") ";
			break;
		case OP_LABEL:
			cout << "LABEL_" << statement.jmpBlock << ":";
			break;
		case OP_EXTLOW64:
			cout << " EXTLOW64";
			break;
		case OP_EXTHIGH64:
			cout << " EXTHIGH64";
			break;
		case OP_FP_RCPL:
			cout << " RCPL";
			break;
		default:
			cout << " ?? ";
			break;
		}

		if(statement.src2)
		{
			cout << statement.src2->ToString();
		}

		cout << endl;
	}
#endif
}

uint32 CJitter::CreateBlock()
{
	uint32 newId = m_nextBlockId++;
	m_basicBlocks[newId] = BASIC_BLOCK();
	return newId;
}

CJitter::BASIC_BLOCK* CJitter::GetBlock(uint32 blockId)
{
	BasicBlockList::iterator blockIterator(m_basicBlocks.find(blockId));
	if(blockIterator == m_basicBlocks.end()) return NULL;
	return &blockIterator->second;
}

void CJitter::InsertStatement(const STATEMENT& statement)
{
	m_currentBlock->statements.push_back(statement);
}

SymbolPtr CJitter::MakeSymbol(SYM_TYPE type, uint32 value)
{
	return MakeSymbol(m_currentBlock, type, value, 0);
}

SymbolPtr CJitter::MakeConstant64(uint64 value)
{
	uint32 valueLo = static_cast<uint32>(value);
	uint32 valueHi = static_cast<uint32>(value >> 32);
	return MakeSymbol(m_currentBlock, SYM_CONSTANT64, valueLo, valueHi);
}

SymbolPtr CJitter::MakeSymbol(BASIC_BLOCK* basicBlock, SYM_TYPE type, uint32 valueLo, uint32 valueHi)
{
	CSymbolTable& currentSymbolTable(basicBlock->symbolTable);
	return currentSymbolTable.MakeSymbol(type, valueLo, valueHi);
}

int CJitter::GetSymbolSize(const SymbolRefPtr& symbolRef)
{
	return symbolRef->GetSymbol().get()->GetSize();
}

SymbolRefPtr CJitter::MakeSymbolRef(const SymbolPtr& symbol)
{
	return SymbolRefPtr(new CSymbolRef(symbol));
}

bool CJitter::FoldConstantOperation(STATEMENT& statement)
{
	CSymbol* src1cst = dynamic_symbolref_cast(SYM_CONSTANT, statement.src1);
	CSymbol* src2cst = dynamic_symbolref_cast(SYM_CONSTANT, statement.src2);

	//Nothing we can do
	if(src1cst == NULL && src2cst == NULL) return false;

	bool changed = false;

	if(statement.op == OP_ADD)
	{
		if(src2cst && src2cst->m_valueLow == 0)
		{
			//Adding with zero
			statement.op = OP_MOV;
			statement.src2.reset();
			changed = true;
		}
		if(src1cst && src2cst)
		{
			//Adding 2 constants
			uint32 result = src1cst->m_valueLow + src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SUB)
	{
		if(src1cst && src2cst)
		{
			//2 constants
			uint32 result = src1cst->m_valueLow - src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_AND)
	{
		if(src1cst && src2cst)
		{
			//2 constants
			uint32 result = src1cst->m_valueLow & src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_OR)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow | src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_NOT)
	{
		if(src1cst)
		{
			uint32 result = ~src1cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SRA)
	{
		if(src1cst && src2cst)
		{
			uint32 result = static_cast<int32>(src1cst->m_valueLow) >> static_cast<int32>(src2cst->m_valueLow);
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_SLL)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow << src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_MUL)
	{
		if(src1cst && src2cst)
		{
			uint64 result = static_cast<uint64>(src1cst->m_valueLow) * static_cast<uint64>(src2cst->m_valueLow);
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(result));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_MULS)
	{
		if(src1cst && src2cst)
		{
			int64 result = static_cast<int64>(static_cast<int32>(src1cst->m_valueLow)) * static_cast<int64>(static_cast<int32>(src2cst->m_valueLow));
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeConstant64(static_cast<uint64>(result)));
			statement.src2.reset();
			changed = true;
		}
	}
	else if(statement.op == OP_DIV)
	{
		if(src1cst && src2cst)
		{
			uint32 result = src1cst->m_valueLow / src2cst->m_valueLow;
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, result));
			statement.src2.reset();
			changed = true;
		}
		else if(src2cst && IsPowerOfTwo(src2cst->m_valueLow))
		{
			statement.op = OP_SRL;
			src2cst->m_valueLow = GetPowerOf2(src2cst->m_valueLow);
		}
	}
	else if(statement.op == OP_CONDJMP)
	{
		if(src1cst && src2cst)
		{
			bool result = false;
			switch(statement.jmpCondition)
			{
			case CONDITION_NE:
				result = src1cst->m_valueLow != src2cst->m_valueLow;
				break;
			default:
				assert(0);
				break;
			}
			changed = true;
			if(result)
			{
				statement.op = OP_JMP;
			}
			else
			{
				statement.op = OP_NOP;
			}
			statement.src1.reset();
			statement.src2.reset();
		}
	}

	return changed;
}

bool CJitter::FoldConstant64Operation(STATEMENT& statement)
{
	CSymbol* src1cst = dynamic_symbolref_cast(SYM_CONSTANT64, statement.src1);
	CSymbol* src2cst = dynamic_symbolref_cast(SYM_CONSTANT64, statement.src2);

	//Nothing we can do
	if(src1cst == NULL && src2cst == NULL) return false;

	bool changed = false;

	if(statement.op == OP_EXTLOW64)
	{
		if(src1cst)
		{
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, src1cst->m_valueLow));
			changed = true;
		}
	}
	else if(statement.op == OP_EXTHIGH64)
	{
		if(src1cst)
		{
			statement.op = OP_MOV;
			statement.src1 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, src1cst->m_valueHigh));
			changed = true;
		}
	}

	return changed;
}

bool CJitter::ConstantFolding(StatementList& statements)
{
	bool changed = false;

	for(StatementList::iterator statementIterator(statements.begin());
		statements.end() != statementIterator; statementIterator++)
	{
		STATEMENT& statement(*statementIterator);
		changed |= FoldConstantOperation(statement);
		changed |= FoldConstant64Operation(statement);
	}
	return changed;
}

void CJitter::MergeBasicBlocks(BASIC_BLOCK& dstBlock, const BASIC_BLOCK& srcBlock)
{
	const StatementList& srcStatements(srcBlock.statements);
	CSymbolTable& dstSymbolTable(dstBlock.symbolTable);

	for(StatementList::const_iterator statementIterator(srcStatements.begin());
		statementIterator != srcStatements.end(); statementIterator++)
	{
		STATEMENT statement(*statementIterator);

		if(statement.dst)
		{
			SymbolPtr symbol(statement.dst->GetSymbol());
			statement.dst = SymbolRefPtr(new CSymbolRef(dstSymbolTable.MakeSymbol(symbol)));
		}

		if(statement.src1)
		{
			SymbolPtr symbol(statement.src1->GetSymbol());
			statement.src1 = SymbolRefPtr(new CSymbolRef(dstSymbolTable.MakeSymbol(symbol)));
		}

		if(statement.src2)
		{
			SymbolPtr symbol(statement.src2->GetSymbol());
			statement.src2 = SymbolRefPtr(new CSymbolRef(dstSymbolTable.MakeSymbol(symbol)));
		}

		dstBlock.statements.push_back(statement);
	}

	dstBlock.optimized = false;
}

CJitter::BASIC_BLOCK CJitter::ConcatBlocks(const BasicBlockList& blocks)
{
	BASIC_BLOCK result;
	for(BasicBlockList::const_iterator blockIterator(blocks.begin());
		blockIterator != blocks.end(); blockIterator++)
	{
		const BASIC_BLOCK& basicBlock(blockIterator->second);
		const StatementList& statements(basicBlock.statements);

		//First, add a mark label statement
		STATEMENT labelStatement;
		labelStatement.op		= OP_LABEL;
		labelStatement.jmpBlock	= blockIterator->first;
		result.statements.push_back(labelStatement);

		MergeBasicBlocks(result, basicBlock);
	}
	return result;
}

bool CJitter::PruneBlocks()
{
	bool changed = true;
	int deletedBlocks = 0;
	while(changed)
	{
		changed = false;

		int toDelete = -1;
		for(BasicBlockList::const_iterator outerBlockIterator(m_basicBlocks.begin());
			outerBlockIterator != m_basicBlocks.end(); outerBlockIterator++)
		{
			//First block is always referenced
			if(outerBlockIterator == m_basicBlocks.begin()) continue;

			unsigned int blockId = outerBlockIterator->first;
			bool referenced = false;

			//Check if there's a reference to this block in here
			for(BasicBlockList::const_iterator innerBlockIterator(m_basicBlocks.begin());
				innerBlockIterator != m_basicBlocks.end(); innerBlockIterator++)
			{
				const BASIC_BLOCK& block(innerBlockIterator->second);
				if(block.statements.size() == 0) continue;

				//Check if this block references the next one or if it jumps to another one
				StatementList::const_iterator lastInstruction(block.statements.end());
				lastInstruction--;
				const STATEMENT& statement(*lastInstruction);

				//It jumps to a block, so check if it references the one we're looking for
				if(statement.op == OP_JMP || statement.op == OP_CONDJMP)
				{
					if(statement.jmpBlock == blockId)
					{
						referenced = true;
						break;
					}
				}

				//Otherwise, it references the next one if it's not a jump
				if(statement.op != OP_JMP)
				{
					BasicBlockList::const_iterator nextBlockIterator(innerBlockIterator);
					nextBlockIterator++;
					if(nextBlockIterator != m_basicBlocks.end())
					{
						if(nextBlockIterator->first == blockId)
						{
							referenced = true;
							break;
						}
					}
				}
			}

			if(!referenced)
			{
				toDelete = blockId;
				break;
			}
		}
		if(toDelete == -1) continue;

		m_basicBlocks.erase(toDelete);
		deletedBlocks++;
		changed = true;
	}

	HarmonizeBlocks();
	return deletedBlocks != 0;
}

void CJitter::HarmonizeBlocks()
{
	//Remove any jumps that jump to the next block
	for(BasicBlockList::iterator blockIterator(m_basicBlocks.begin());
		blockIterator != m_basicBlocks.end(); blockIterator++)
	{
		BasicBlockList::iterator nextBlockIterator(blockIterator);
		nextBlockIterator++;
		if(nextBlockIterator == m_basicBlocks.end()) continue;

		BASIC_BLOCK& basicBlock(blockIterator->second);
		BASIC_BLOCK& nextBlock(nextBlockIterator->second);

		StatementList::const_iterator lastStatementIterator(basicBlock.statements.end());
		lastStatementIterator--;
		const STATEMENT& statement(*lastStatementIterator);
		if(statement.op != OP_JMP) continue;
		if(statement.jmpBlock != nextBlockIterator->first) continue;

		//Remove the jump
		basicBlock.statements.erase(lastStatementIterator);
	}

	//Flag any block that have a reference from a jump
	for(BasicBlockList::iterator outerBlockIterator(m_basicBlocks.begin());
		outerBlockIterator != m_basicBlocks.end(); outerBlockIterator++)
	{
		BASIC_BLOCK& outerBlock(outerBlockIterator->second);
		outerBlock.hasJumpRef = false;

		for(BasicBlockList::const_iterator innerBlockIterator(m_basicBlocks.begin());
			innerBlockIterator != m_basicBlocks.end(); innerBlockIterator++)
		{
			const BASIC_BLOCK& block(innerBlockIterator->second);
			if(block.statements.size() == 0) continue;

			//Check if this block references the next one or if it jumps to another one
			StatementList::const_iterator lastInstruction(block.statements.end());
			lastInstruction--;
			const STATEMENT& statement(*lastInstruction);

			//It jumps to a block, so check if it references the one we're looking for
			if(statement.op == OP_JMP || statement.op == OP_CONDJMP)
			{
				if(statement.jmpBlock == outerBlockIterator->first)
				{
					outerBlock.hasJumpRef = true;
					break;
				}
			}
		}
	}
}

bool CJitter::MergeBlocks()
{
	int deletedBlocks = 0;
	bool changed = true;
	while(changed)
	{
		changed = false;
		for(BasicBlockList::iterator blockIterator(m_basicBlocks.begin());
			m_basicBlocks.end() != blockIterator; blockIterator++)
		{
			BasicBlockList::iterator nextBlockIterator(blockIterator);
			nextBlockIterator++;
			if(nextBlockIterator == m_basicBlocks.end()) continue;

			BASIC_BLOCK& basicBlock(blockIterator->second);
			BASIC_BLOCK& nextBlock(nextBlockIterator->second);

			if(nextBlock.hasJumpRef) continue;

			//Check if the last statement is a jump
			StatementList::const_iterator lastStatementIterator(basicBlock.statements.end());
			lastStatementIterator--;
			const STATEMENT& statement(*lastStatementIterator);
			if(statement.op == OP_CONDJMP) continue;
			if(statement.op == OP_JMP) continue;

			//Blocks can be merged
			MergeBasicBlocks(basicBlock, nextBlock);

			m_basicBlocks.erase(nextBlockIterator);

			deletedBlocks++;
			changed = true;
			break;
		}
	}
	return deletedBlocks != 0;
}

bool CJitter::ConstantPropagation(StatementList& statements)
{
	bool changed = false;

	for(StatementList::iterator outerStatementIterator(statements.begin());
		statements.end() != outerStatementIterator; outerStatementIterator++)
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		if(outerStatement.op != OP_MOV) continue;

		CSymbol* constant = dynamic_symbolref_cast(SYM_CONSTANT, outerStatement.src1);
		if(constant == NULL)
		{
			constant = dynamic_symbolref_cast(SYM_CONSTANT64, outerStatement.src1);
		}
		if(!constant) continue;

		//Find anything that uses this operand and replace it with the constant
		for(StatementList::iterator innerStatementIterator(outerStatementIterator);
			statements.end() != innerStatementIterator; innerStatementIterator++)
		{
			if(outerStatementIterator == innerStatementIterator) continue;

			STATEMENT& innerStatement(*innerStatementIterator);

			if(innerStatement.src1 && innerStatement.src1->Equals(outerStatement.dst.get()))
			{
				innerStatement.src1 = outerStatement.src1;
				changed = true;
				continue;
			}

			if(innerStatement.src2 && innerStatement.src2->Equals(outerStatement.dst.get()))
			{
				innerStatement.src2 = outerStatement.src1;
				changed = true;
				continue;
			}
		}
	}
	return changed;
}

bool CJitter::CopyPropagation(StatementList& statements)
{
	bool changed = false;

	for(StatementList::iterator outerStatementIterator(statements.begin());
		statements.end() != outerStatementIterator; outerStatementIterator++)
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		//Some operations we can't propagate
		if(outerStatement.op == OP_RETVAL) continue;

		//Check for all OP_MOVs that uses the result of this operation and propagate
		for(StatementList::iterator innerStatementIterator(statements.begin());
			statements.end() != innerStatementIterator; innerStatementIterator++)
		{
			if(outerStatementIterator == innerStatementIterator) continue;

			STATEMENT& innerStatement(*innerStatementIterator);

			if(innerStatement.op == OP_MOV && innerStatement.src1->Equals(outerStatement.dst.get()))
			{
				innerStatement.op = outerStatement.op;
				innerStatement.src1 = outerStatement.src1;
				innerStatement.src2 = outerStatement.src2;
				changed = true;
			}
		}
	}
	return changed;
}

bool CJitter::DeadcodeElimination(VERSIONED_STATEMENT_LIST& versionedStatementList)
{
	bool changed = false;

	typedef std::list<StatementList::iterator> ToDeleteList;
	ToDeleteList toDelete;

	for(StatementList::iterator outerStatementIterator(versionedStatementList.statements.begin());
		versionedStatementList.statements.end() != outerStatementIterator; outerStatementIterator++)
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		CSymbol* candidate = NULL;
		if(CSymbol* tempSymbol = dynamic_symbolref_cast(SYM_TEMPORARY, outerStatement.dst))
		{
			candidate = tempSymbol;
		}
		else if(CSymbol* tempSymbol = dynamic_symbolref_cast(SYM_TEMPORARY64, outerStatement.dst))
		{
			candidate = tempSymbol;
		}
		else if(CSymbol* tempSymbol = dynamic_symbolref_cast(SYM_FP_TMP_SINGLE, outerStatement.dst))
		{
			candidate = tempSymbol;
		}
		else if(CSymbol* relativeSymbol = dynamic_symbolref_cast(SYM_RELATIVE, outerStatement.dst))
		{
			VersionedSymbolRefPtr versionedSymbolRef = std::tr1::dynamic_pointer_cast<CVersionedSymbolRef>(outerStatement.dst);
			assert(versionedSymbolRef);
			if(versionedSymbolRef->version != versionedStatementList.relativeVersions.GetRelativeVersion(relativeSymbol->m_valueLow))
			{
				candidate = relativeSymbol;
			}
		}

		if(candidate == NULL) continue;

		//Look for any possible use of this symbol
		bool used = false;
		for(StatementList::iterator innerStatementIterator(outerStatementIterator);
			versionedStatementList.statements.end() != innerStatementIterator; innerStatementIterator++)
		{
			if(outerStatementIterator == innerStatementIterator) continue;

			STATEMENT& innerStatement(*innerStatementIterator);

			if(innerStatement.src1)
			{
				SymbolPtr symbol(innerStatement.src1->GetSymbol());
				if(symbol->Equals(candidate))
				{
					used = true;
					break;
				}
				if(symbol->Aliases(candidate))
				{
					used = true;
					break;
				}
			}
			if(innerStatement.src2)
			{
				SymbolPtr symbol(innerStatement.src2->GetSymbol());
				if(symbol->Equals(candidate))
				{
					used = true;
					break;
				}
				if(symbol->Aliases(candidate))
				{
					used = true;
					break;
				}
			}
		}

		if(!used)
		{
			//Kill it!
			toDelete.push_back(outerStatementIterator);
		}
	}

	for(ToDeleteList::const_iterator deleteIterator(toDelete.begin());
		toDelete.end() != deleteIterator; deleteIterator++)
	{
		versionedStatementList.statements.erase(*deleteIterator);
		changed = true;
	}

	return changed;
}

void CJitter::CoalesceTemporaries(BASIC_BLOCK& basicBlock)
{
	typedef std::vector<CSymbol*> EncounteredTempList;
	EncounteredTempList encounteredTemps;

	for(StatementList::iterator outerStatementIterator(basicBlock.statements.begin());
		basicBlock.statements.end() != outerStatementIterator; outerStatementIterator++)
	{
		STATEMENT& outerStatement(*outerStatementIterator);

		CSymbol* tempSymbol = dynamic_symbolref_cast(SYM_TEMPORARY, outerStatement.dst);
		if(tempSymbol == NULL) continue;

		CSymbol* candidate = NULL;

		//Check for a possible replacement
		for(EncounteredTempList::const_iterator tempIterator(encounteredTemps.begin());
			tempIterator != encounteredTemps.end(); tempIterator++)
		{
			//Look for any possible use of this symbol
			CSymbol* encounteredTemp = *tempIterator;
			bool used = false;

			for(StatementList::iterator innerStatementIterator(outerStatementIterator);
				basicBlock.statements.end() != innerStatementIterator; innerStatementIterator++)
			{
				if(outerStatementIterator == innerStatementIterator) continue;

				STATEMENT& innerStatement(*innerStatementIterator);

				if(innerStatement.dst)
				{
					SymbolPtr symbol(innerStatement.dst->GetSymbol());
					if(symbol->Equals(encounteredTemp))
					{
						used = true;
						break;
					}
				}
				if(innerStatement.src1)
				{
					SymbolPtr symbol(innerStatement.src1->GetSymbol());
					if(symbol->Equals(encounteredTemp))
					{
						used = true;
						break;
					}
				}
				if(innerStatement.src2)
				{
					SymbolPtr symbol(innerStatement.src2->GetSymbol());
					if(symbol->Equals(encounteredTemp))
					{
						used = true;
						break;
					}
				}
			}

			if(!used)
			{
				candidate = encounteredTemp;
				break;
			}
		}

		if(candidate == NULL)
		{
			encounteredTemps.push_back(tempSymbol);
		}
		else
		{
			SymbolPtr candidatePtr = MakeSymbol(candidate->m_type, candidate->m_valueLow);

			outerStatement.dst = MakeSymbolRef(candidatePtr);

			//Replace all occurences of this temp with the candidate
			for(StatementList::iterator innerStatementIterator(outerStatementIterator);
				basicBlock.statements.end() != innerStatementIterator; innerStatementIterator++)
			{
				if(outerStatementIterator == innerStatementIterator) continue;

				STATEMENT& innerStatement(*innerStatementIterator);

				if(innerStatement.dst)
				{
					SymbolPtr symbol(innerStatement.dst->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.dst = MakeSymbolRef(candidatePtr);
					}
				}
				if(innerStatement.src1)
				{
					SymbolPtr symbol(innerStatement.src1->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.src1 = MakeSymbolRef(candidatePtr);
					}
				}
				if(innerStatement.src2)
				{
					SymbolPtr symbol(innerStatement.src2->GetSymbol());
					if(symbol->Equals(tempSymbol))
					{
						innerStatement.src2 = MakeSymbolRef(candidatePtr);
					}
				}
			}
		}
	}
}

void CJitter::ComputeLivenessAndPruneSymbols(BASIC_BLOCK& basicBlock)
{
	CSymbolTable& symbolTable(basicBlock.symbolTable);
	const StatementList& statements(basicBlock.statements);

	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		SymbolPtr& symbol(*symbolIterator);
		symbol->m_useCount = 0;

		unsigned int statementIdx = 0;
		for(StatementList::const_iterator statementIterator(statements.begin());
			statementIterator != statements.end(); statementIterator++, statementIdx++)
		{
			const STATEMENT& statement(*statementIterator);
			if(statement.dst)
			{
				SymbolPtr dstSymbol(statement.dst->GetSymbol());
				if(dstSymbol->Equals(symbol.get()))
				{
					symbol->m_useCount++;
					if(symbol->m_firstDef == -1)
					{
						symbol->m_firstDef = statementIdx;
					}
				}
				else if(!symbol->m_aliased && dstSymbol->Aliases(symbol.get()))
				{
					symbol->m_aliased = true;
				}
			}

			if(statement.src1)
			{
				SymbolPtr src1Symbol(statement.src1->GetSymbol());
				if(src1Symbol->Equals(symbol.get()))
				{
					symbol->m_useCount++;
					if(symbol->m_firstUse == -1)
					{
						symbol->m_firstUse = statementIdx;
					}
				}
				else if(!symbol->m_aliased && src1Symbol->Aliases(symbol.get()))
				{
					symbol->m_aliased = true;
				}
			}

			if(statement.src2)
			{
				SymbolPtr src2Symbol(statement.src2->GetSymbol());
				if(src2Symbol->Equals(symbol.get()))
				{
					symbol->m_useCount++;
					if(symbol->m_firstUse == -1)
					{
						symbol->m_firstUse = statementIdx;
					}
				}
				else if(!symbol->m_aliased && src2Symbol->Aliases(symbol.get()))
				{
					symbol->m_aliased = true;
				}
			}
		}
	}

	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd();)
	{
		const SymbolPtr& symbol(*symbolIterator);
		if(symbol->m_useCount == 0)
		{
			symbolIterator = symbolTable.RemoveSymbol(symbolIterator);
		}
		else
		{
			symbolIterator++;
		}
	}
}

unsigned int CJitter::AllocateStack(BASIC_BLOCK& basicBlock)
{
	unsigned int stackAlloc = 0;
	CSymbolTable& symbolTable(basicBlock.symbolTable);
	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		const SymbolPtr& symbol(*symbolIterator);
		if(symbol->m_register != -1) continue;

		if(symbol->m_type == SYM_TEMPORARY)
		{
			symbol->m_stackLocation = stackAlloc;
			stackAlloc += 4;
		}
		else if(symbol->m_type == SYM_TEMPORARY64)
		{
			assert((stackAlloc & 7) == 0);
			symbol->m_stackLocation = stackAlloc;
			stackAlloc += 8;
		}
	}
	return stackAlloc;
}

void CJitter::AllocateRegisters(BASIC_BLOCK& basicBlock)
{
	if(basicBlock.statements.size() == 0) return;

	unsigned int regCount = m_codeGen->GetAvailableRegisterCount();
	unsigned int currentRegister = 0;
	CSymbolTable& symbolTable(basicBlock.symbolTable);
	StatementList& statements(basicBlock.statements);

	struct UseCountSymbolComparator
	{
		bool operator()(const CSymbol* symbol1, const CSymbol* symbol2)
		{
			return symbol1->m_useCount > symbol2->m_useCount;
		}
	};
	typedef std::list<CSymbol*> UseCountSymbolSortedList;

	UseCountSymbolSortedList sortedSymbols;
	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		sortedSymbols.push_back(symbolIterator->get());
	}
	sortedSymbols.sort(UseCountSymbolComparator());

	for(unsigned int i = 0; i < regCount; i++)
	{
		symbolTable.MakeSymbol(SYM_REGISTER, i);
	}

	UseCountSymbolSortedList::iterator symbolIterator = sortedSymbols.begin();
	while(1)
	{
		if(symbolIterator == sortedSymbols.end())
		{
			//We're done
			break;
		}

		if(currentRegister == regCount)
		{
			//We're done
			break;
		}

		CSymbol* symbol(*symbolIterator);
		if(
			((symbol->m_type == SYM_RELATIVE) && (symbol->m_useCount != 0) && (!symbol->m_aliased)) ||
			((symbol->m_type == SYM_TEMPORARY) && (symbol->m_useCount != 0))
			)
		{
			symbol->m_register = currentRegister;

			//Replace all uses of this symbol with register
			for(StatementList::iterator statementIterator(statements.begin());
				statementIterator != statements.end(); statementIterator++)
			{
				STATEMENT& statement(*statementIterator);
				if(statement.dst && statement.dst->GetSymbol()->Equals(symbol))
				{
					statement.dst = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, currentRegister));
				}

				if(statement.src1 && statement.src1->GetSymbol()->Equals(symbol))
				{
					statement.src1 = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, currentRegister));
				}

				if(statement.src2 && statement.src2->GetSymbol()->Equals(symbol))
				{
					statement.src2 = MakeSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, currentRegister));
				}
			}

			currentRegister++;
		}

		symbolIterator++;
	}

	//Find the final instruction where to dump registers to
	StatementList::const_iterator endInsertionPoint(statements.end());
	{
		StatementList::const_iterator endInstructionIterator(statements.end());
		endInstructionIterator--;
		const STATEMENT& statement(*endInstructionIterator);
		if(statement.op == OP_CONDJMP || statement.op == OP_JMP)
		{
			endInsertionPoint--;
		}
	}

	//Emit copies to registers
	for(CSymbolTable::SymbolIterator symbolIterator(symbolTable.GetSymbolsBegin());
		symbolIterator != symbolTable.GetSymbolsEnd(); symbolIterator++)
	{
		const SymbolPtr& symbol(*symbolIterator);
		if(symbol->m_register == -1) continue;
		if(symbol->m_type == SYM_TEMPORARY) continue;

		//We use this symbol before we define it, so we need to load it first
		if(symbol->m_firstUse != -1 && symbol->m_firstUse <= symbol->m_firstDef)
		{
			STATEMENT statement;
			statement.op	= OP_MOV;
			statement.dst	= SymbolRefPtr(new CSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, symbol->m_register)));
			statement.src1	= SymbolRefPtr(new CSymbolRef(symbol));

			statements.push_front(statement);
		}

		//If symbol is defined, we need to save it at the end
		if(symbol->m_firstDef != -1)
		{
			STATEMENT statement;
			statement.op	= OP_MOV;
			statement.dst	= SymbolRefPtr(new CSymbolRef(symbol));
			statement.src1	= SymbolRefPtr(new CSymbolRef(symbolTable.MakeSymbol(SYM_REGISTER, symbol->m_register)));

			statements.insert(endInsertionPoint, statement);
		}
	}
}
