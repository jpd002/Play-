#pragma once

#include <thread>
#include <deque>
#include <boost/filesystem.hpp>
#include "LockFreeQueue.h"
#include "Playlist.h"
#include "PsfTags.h"

class CPlaylistDiscoveryService
{
public:
									CPlaylistDiscoveryService();
	virtual							~CPlaylistDiscoveryService();
	
	void							AddItemInRun(const boost::filesystem::path& filePath, const boost::filesystem::path& archivePath, unsigned int itemId);
	void							ResetRun();
	void							ProcessPendingItems(CPlaylist&);
	
private:
	struct COMMAND
	{
		boost::filesystem::path		filePath;
		boost::filesystem::path		archivePath;
		unsigned int				runId;
		unsigned int				itemId;
	};
	
	struct RESULT
	{
		unsigned int	runId;
		unsigned int	itemId;
		std::wstring	title;
		double			length;
	};
	
	typedef std::deque<COMMAND> CommandQueue;
	typedef std::deque<RESULT> ResultQueue;
	
	void							ThreadProc();
	
	std::thread						m_thread;
	bool							m_threadActive;
	CLockFreeQueue<COMMAND>			m_commandQueue;
	CLockFreeQueue<RESULT>			m_resultQueue;
	CommandQueue					m_pendingCommands;
	uint32							m_runId;
	CPsfTags::CHAR_ENCODING			m_charEncoding;
};
