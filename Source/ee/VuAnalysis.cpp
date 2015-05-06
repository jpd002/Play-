#include "VuAnalysis.h"
#include "../MIPS.h"
#include "../Ps2Const.h"
#include "VUShared.h"

void CVuAnalysis::Analyse(CMIPS* ctx, uint32 begin, uint32 end)
{
	int routineCount = 0;

	begin &= ~0x07;
	end &= ~0x07;

	std::set<uint32> subroutineAddresses;

	//First pass: Check for BAL
	for(uint32 address = begin; address <= end; address += 8)
	{
		uint32 lowerInstruction = ctx->m_pMemoryMap->GetInstruction(address + 0);
		uint32 upperInstruction = ctx->m_pMemoryMap->GetInstruction(address + 4);

		//Check for LOI (skip)
		if(upperInstruction & 0x80000000) continue;

		//Check for BAL
		if((lowerInstruction & 0xFE000000) == (0x21 << 25))
		{
			uint32 jumpTarget = address + VUShared::GetBranch(lowerInstruction & 0x07FF) + 8;
			if(jumpTarget < begin) continue;
			if(jumpTarget >= end) continue;
			subroutineAddresses.insert(jumpTarget);
		}
	}

	//Second pass: Check for END bit and JR
	uint32 potentialRoutineStart = 0;
	for(uint32 address = begin; address <= end; address += 8)
	{
		uint32 lowerInstruction = ctx->m_pMemoryMap->GetInstruction(address + 0);
		uint32 upperInstruction = ctx->m_pMemoryMap->GetInstruction(address + 4);

		if(lowerInstruction == 0 && upperInstruction == 0)
		{
			potentialRoutineStart = address + 8;
			continue;
		}

		//Check for LOI (skip)
		if(upperInstruction & 0x80000000) continue;

		//Check for JR or END bit
		if(
			(lowerInstruction & 0xFE000000) == (0x24 << 25) ||
			(upperInstruction & 0x40000000)
			)
		{
			subroutineAddresses.insert(potentialRoutineStart);
			potentialRoutineStart = address + 8;
		}
	}

	//Create subroutines
	for(const auto& subroutineAddress : subroutineAddresses)
	{
		//Don't bother if we already found it
		if(ctx->m_analysis->FindSubroutine(subroutineAddress)) continue;

		//Otherwise, try to find a function that already exists
		for(uint32 address = subroutineAddress; address <= end; address += 8)
		{
			uint32 lowerInstruction = ctx->m_pMemoryMap->GetInstruction(address + 0);
			uint32 upperInstruction = ctx->m_pMemoryMap->GetInstruction(address + 4);

			//Check for LOI (skip)
			if(upperInstruction & 0x80000000) continue;

			//Check for JR or END bit
			if(
				(lowerInstruction & 0xFE000000) == (0x24 << 25) ||
				(upperInstruction & 0x40000000)
				)
			{
				ctx->m_analysis->InsertSubroutine(subroutineAddress, address + 8, 0, 0, 0, 0);
				routineCount++;
				break;
			}

			auto subroutine = ctx->m_analysis->FindSubroutine(address);
			if(subroutine)
			{
				//Function already exists, merge.
				ctx->m_analysis->ChangeSubroutineStart(subroutine->start, subroutineAddress);
				break;
			}
		}
	}

	//Find orphaned branches
	for(uint32 address = begin; address <= end; address += 8)
	{
		//Address already associated with subroutine, don't bother
		if(ctx->m_analysis->FindSubroutine(address)) continue;

		uint32 lowerInstruction = ctx->m_pMemoryMap->GetInstruction(address + 0);
		uint32 upperInstruction = ctx->m_pMemoryMap->GetInstruction(address + 4);

		auto branchType = ctx->m_pArch->IsInstructionBranch(ctx, address, lowerInstruction);
		if(branchType == MIPS_BRANCH_NORMAL)
		{
			uint32 branchTarget = ctx->m_pArch->GetInstructionEffectiveAddress(ctx, address, lowerInstruction);
			if(branchTarget != 0)
			{
				auto subroutine = ctx->m_analysis->FindSubroutine(branchTarget);
				if(subroutine)
				{
					ctx->m_analysis->ChangeSubroutineEnd(subroutine->start, address + 8);
				}
			}
		}
	}
}
