#include "GameConfig.h"
#include "xml/Node.h"
#include "xml/Parser.h"
#include "xml/FilteringNodeIterator.h"
#include "PathUtils.h"
#include "StdStreamUtils.h"
#ifdef __ANDROID__
#include "android/AssetStream.h"
#endif
#include "Log.h"
#include "PS2VM.h"
#include "ee/EeExecutor.h"

#define LOG_NAME "GameConfig"

#define GAMECONFIG_FILENAME "GameConfig.xml"

void CGameConfig::ApplyGameConfig(CPS2VM& virtualMachine)
{
	std::unique_ptr<Framework::Xml::CNode> document;
	try
	{
#ifdef __ANDROID__
		Framework::Android::CAssetStream inputStream(GAMECONFIG_FILENAME);
#else
		auto gameConfigPath = Framework::PathUtils::GetAppResourcesPath() / GAMECONFIG_FILENAME;
		Framework::CStdStream inputStream(Framework::CreateInputStdStream(gameConfigPath.native()));
#endif
		document = Framework::Xml::CParser::ParseDocument(inputStream);
		if(!document) return;
	}
	catch(const std::exception& exception)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Failed to open game config file: %s.\r\n", exception.what());
		return;
	}

	auto gameConfigsNode = document->Select("GameConfigs");
	if(!gameConfigsNode)
	{
		return;
	}

	CEeExecutor::BlockFpRoundingModeMap blockFpRoundingModes;
	CEeExecutor::BlockFpUseAccurateAddSubSet blockFpUseAccurateAddSub;
	CEeExecutor::IdleLoopBlockMap idleLoopBlocks;

	const char* executableName = virtualMachine.m_ee->m_os->GetExecutableName();
	auto eeExecutor = static_cast<CEeExecutor*>(virtualMachine.m_ee->m_EE.m_executor.get());
	auto eeRam = virtualMachine.m_ee->m_ram;

	for(Framework::Xml::CFilteringNodeIterator itNode(gameConfigsNode, "GameConfig");
	    !itNode.IsEnd(); itNode++)
	{
		auto gameConfigNode = (*itNode);

		const char* executable = gameConfigNode->GetAttribute("Executable");
		if(!executable) continue;

		if(strcmp(executable, executableName)) continue;

		//Found the right executable, apply config

		for(Framework::Xml::CFilteringNodeIterator itNode(gameConfigNode, "Patch");
		    !itNode.IsEnd(); itNode++)
		{
			auto patch = (*itNode);

			const char* addressString = patch->GetAttribute("Address");
			const char* valueString = patch->GetAttribute("Value");

			if(!addressString) continue;
			if(!valueString) continue;

			uint32 value = 0, address = 0;
			if(sscanf(addressString, "%x", &address) == 0) continue;
			if(sscanf(valueString, "%x", &value) == 0) continue;

			*reinterpret_cast<uint32*>(eeRam + address) = value;
		}

		for(Framework::Xml::CFilteringNodeIterator itNode(gameConfigNode, "BlockFpRoundingMode");
		    !itNode.IsEnd(); itNode++)
		{
			auto node = (*itNode);

			const char* addressString = node->GetAttribute("Address");
			const char* modeString = node->GetAttribute("Mode");

			if(!addressString) continue;
			if(!modeString) continue;

			uint32 address = 0;
			auto roundingMode = Jitter::CJitter::ROUND_NEAREST;
			if(sscanf(addressString, "%x", &address) == 0) continue;

			if(!strcmp(modeString, "NEAREST"))
			{
				roundingMode = Jitter::CJitter::ROUND_NEAREST;
			}
			else if(!strcmp(modeString, "PLUSINFINITY"))
			{
				roundingMode = Jitter::CJitter::ROUND_PLUSINFINITY;
			}
			else if(!strcmp(modeString, "MINUSINFINITY"))
			{
				roundingMode = Jitter::CJitter::ROUND_MINUSINFINITY;
			}
			else if(!strcmp(modeString, "TRUNCATE"))
			{
				roundingMode = Jitter::CJitter::ROUND_TRUNCATE;
			}
			else
			{
				assert(false);
				continue;
			}

			blockFpRoundingModes.insert(std::make_pair(address, roundingMode));
		}

		for(Framework::Xml::CFilteringNodeIterator itNode(gameConfigNode, "BlockFpUseAccurateAddSub");
		    !itNode.IsEnd(); itNode++)
		{
			auto node = (*itNode);

			const char* addressString = node->GetAttribute("Address");
			if(!addressString) continue;

			uint32 address = 0;
			if(sscanf(addressString, "%x", &address) == 0) continue;

			blockFpUseAccurateAddSub.insert(address);
		}

		for(Framework::Xml::CFilteringNodeIterator itNode(gameConfigNode, "IdleLoopBlock");
		    !itNode.IsEnd(); itNode++)
		{
			auto node = (*itNode);

			const char* addressString = node->GetAttribute("Address");
			const char* checkBlockKeyString = node->GetAttribute("CheckBlockKey");

			if(!addressString) continue;

			uint32 address = 0;
			std::optional<CEeExecutor::CachedBlockKey> checkBlockKey;

			if(sscanf(addressString, "%x", &address) == 0) continue;
			if(checkBlockKeyString)
			{
				CEeExecutor::CachedBlockKey blockKey;
				int parseCount = sscanf(checkBlockKeyString, "%08X%08X%08X%08X;%d",
				                        &blockKey.first.nV[3], &blockKey.first.nV[2],
				                        &blockKey.first.nV[1], &blockKey.first.nV[0],
				                        &blockKey.second);
				if(parseCount != 5)
				{
					continue;
				}
				checkBlockKey = blockKey;
			}

			idleLoopBlocks.insert(std::make_pair(address, checkBlockKey));
		}

		eeExecutor->SetBlockFpRoundingModes(std::move(blockFpRoundingModes));
		eeExecutor->SetBlockFpUseAccurateAddSub(std::move(blockFpUseAccurateAddSub));
		eeExecutor->SetIdleLoopBlocks(std::move(idleLoopBlocks));

		break;
	}
}
