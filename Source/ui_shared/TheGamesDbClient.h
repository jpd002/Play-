#pragma once

#include <string>
#include <vector>
#include "Types.h"
#include "Singleton.h"

namespace TheGamesDb
{
	struct Game
	{
		std::string baseImgUrl;
		std::string title;
		std::string overview;
		std::string boxArtUrl;
	};

	struct GamesListItem
	{
		uint32 id;
		std::string gameTitle;
	};
	typedef std::vector<GamesListItem> GamesList;

	class CClient : public CSingleton<CClient>
	{
	public:
		virtual ~CClient() = default;

		Game      GetGame(uint32 id);
		GamesList GetGamesList(const std::string& platformName, const std::string& name);
	};
}
