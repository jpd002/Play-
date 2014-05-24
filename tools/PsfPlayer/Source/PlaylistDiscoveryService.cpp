#include "PlaylistDiscoveryService.h"
#include "PsfStreamProvider.h"
#include "PsfBase.h"
#include "PsfTags.h"

CPlaylistDiscoveryService::CPlaylistDiscoveryService()
: m_threadActive(false)
, m_commandQueue(5)
, m_resultQueue(5)
, m_runId(0)
, m_charEncoding(CPsfTags::CE_WINDOWS_1252)
{
	m_threadActive = true;
	m_thread = std::thread([&] () { ThreadProc(); });
}

CPlaylistDiscoveryService::~CPlaylistDiscoveryService()
{
	m_threadActive = false;
	m_thread.join();
}

void CPlaylistDiscoveryService::SetCharEncoding(const CPsfTags::CHAR_ENCODING& charEncoding)
{
	m_charEncoding = charEncoding;
}

void CPlaylistDiscoveryService::AddItemInRun(const CPsfPathToken& filePath, const boost::filesystem::path& archivePath, unsigned int itemId)
{
	COMMAND command;
	command.runId		= m_runId;
	command.itemId		= itemId;
	command.filePath	= filePath;
	command.archivePath	= archivePath;
	m_pendingCommands.push_back(command);
}

void CPlaylistDiscoveryService::ResetRun()
{
	m_pendingCommands.clear();
	m_runId++;
}

void CPlaylistDiscoveryService::ProcessPendingItems(CPlaylist& playlist)
{
	while(m_pendingCommands.size() != 0)
	{
		const auto& command(*m_pendingCommands.begin());
		if(m_commandQueue.TryPush(command))
		{
			m_pendingCommands.pop_front();
		}
		else
		{
			break;
		}
	}
	
	{
		RESULT result;
		if(m_resultQueue.TryPop(result))
		{
			if(result.runId == m_runId)
			{
				int itemIdx = playlist.FindItem(result.itemId);
				if(itemIdx != -1)
				{
					CPlaylist::ITEM item = playlist.GetItem(itemIdx);
					item.title	= result.title;
					item.length	= result.length;
					playlist.UpdateItem(itemIdx, item);
				}
			}
		}
	}
}

void CPlaylistDiscoveryService::ThreadProc()
{
	ResultQueue pendingResults;
	
	while(m_threadActive)
	{
		COMMAND command;
		if(m_commandQueue.TryPop(command))
		{
			try
			{
				auto streamProvider = CreatePsfStreamProvider(command.archivePath);
				std::unique_ptr<Framework::CStream> inputStream(streamProvider->GetStreamForPath(command.filePath));
				CPsfBase psfFile(*inputStream);
				CPsfTags tags(CPsfTags::TagMap(psfFile.GetTagsBegin(), psfFile.GetTagsEnd()));
				tags.SetDefaultCharEncoding(m_charEncoding);
				
				RESULT result;
				result.runId = command.runId;
				result.itemId = command.itemId;
				result.length = 0;
				
				if(tags.HasTag("title"))
				{
					result.title = tags.GetTagValue("title");
				}
				else
				{
					result.title = command.filePath.GetWidePath();
				}
				
				if(tags.HasTag("length"))
				{
					result.length = tags.ConvertTimeString(tags.GetTagValue("length").c_str());
				}
								
				pendingResults.push_back(result);
			}
			catch(...)
			{
				//assert(0);
			}
		}
		
		while(pendingResults.size() != 0)
		{
			const auto& command(*pendingResults.begin());
			if(m_resultQueue.TryPush(command))
			{
				pendingResults.pop_front();
			}
			else
			{
				break;
			}
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
