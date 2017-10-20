#pragma once

#include <string>
#include "Types.h"
#include "Singleton.h"
#include "sqlite/SqliteDb.h"

namespace LocalGamesDb
{
	struct Game
	{
		uint32 theGamesDbId = 0;
		std::string title;
	};

	class CClient : public CSingleton<CClient>
	{
	public:
		CClient();
		virtual ~CClient() = default;

		Game GetGame(const char*);

	private:
		Framework::CSqliteDb m_db;
	};
};
