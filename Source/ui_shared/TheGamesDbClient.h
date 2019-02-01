#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Types.h"
#include "Singleton.h"

namespace TheGamesDb
{
	struct Game
	{
		uint32 id;
		std::string baseImgUrl;
		std::string title;
		std::string overview;
		std::string boxArtUrl;
		std::vector<std::string> discIds;
	};

	typedef std::vector<Game> GamesList;

	class CClient : public CSingleton<CClient>
	{
	public:
		virtual ~CClient() = default;

		Game GetGame(uint32 id);
		GamesList GetGamesList(const std::string& platformID, const std::string& name);

		GamesList GetGames(std::vector<std::string> serials);

	private:
		GamesList PopulateGameList(const nlohmann::json&);
	};
}
