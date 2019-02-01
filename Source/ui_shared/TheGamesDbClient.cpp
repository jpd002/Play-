#include <memory>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "TheGamesDbClient.h"
#include "string_format.h"
#include "http/HttpClientFactory.h"

using namespace TheGamesDb;

#define API_KEY "API_KEY"

static const char* g_getGameUrl = "https://api.thegamesdb.net/Games/GamesByGameID?apikey=" API_KEY "&fields=overview,uids&include=boxart&id=%d";
static const char* g_getGamesByUIDUrl = "https://api.thegamesdb.net/Games/ByGameUniqueID?apikey=" API_KEY "&filter%5Bplatform%5D=11&fields=overview,uids&include=boxart";
static const char* g_getGamesListUrl = "https://api.thegamesdb.net/v1.1/Games/ByGameName?apikey=" API_KEY "&fields=overview,uids&filter%5Bplatform%5D=%s&include=boxart&name=%s";

GamesList CClient::GetGames(std::vector<std::string> serials)
{
	std::ostringstream stream;
	std::copy(serials.begin(), serials.end(), std::ostream_iterator<std::string>(stream, ","));
	std::string str_games_id = stream.str();

	std::vector<Game> gamesList;

	auto url = std::string(g_getGamesByUIDUrl);
	url += "&uid=";
	url += str_games_id;
	while(!url.empty())
	{
		auto requestResult =
		    [&]() {
			    auto client = Framework::Http::CreateHttpClient();
			    client->SetUrl(url);
			    return client->SendRequest();
		    }();

		url.clear();
		if(requestResult.statusCode == Framework::Http::HTTP_STATUS_CODE::OK)
		{
			auto json_ret = requestResult.data.ReadString();
			auto parsed_json = nlohmann::json::parse(json_ret);

			auto games = PopulateGameList(parsed_json);
			gamesList.insert(gamesList.end(), games.begin(), games.end());

			if(!parsed_json["pages"]["next"].empty())
			{
				url = parsed_json["pages"]["next"].get<std::string>();
			}
		}
	}
	return gamesList;
}

Game CClient::GetGame(uint32 id)
{
	auto url = string_format(g_getGameUrl, id);
	auto requestResult =
	    [&]() {
		    auto client = Framework::Http::CreateHttpClient();
		    client->SetUrl(url);
		    return client->SendRequest();
	    }();

	if(requestResult.statusCode != Framework::Http::HTTP_STATUS_CODE::OK)
	{
		throw std::runtime_error("Failed to get game.");
	}

	auto json_ret = requestResult.data.ReadString();
	auto parsed_json = nlohmann::json::parse(json_ret);

	auto gamesList = PopulateGameList(parsed_json);
	if(gamesList.empty())
	{
		throw std::runtime_error("Failed to get game.");
	}
	return gamesList.at(0);
}

GamesList CClient::GetGamesList(const std::string& platformID, const std::string& name)
{
	auto encodedName = Framework::Http::CHttpClient::UrlEncode(name);

	auto url = string_format(g_getGamesListUrl, platformID.c_str(), encodedName.c_str());
	auto requestResult =
	    [&]() {
		    auto client = Framework::Http::CreateHttpClient();
		    client->SetUrl(url);
		    return client->SendRequest();
	    }();

	if(requestResult.statusCode != Framework::Http::HTTP_STATUS_CODE::OK)
	{
		throw std::runtime_error("Failed to get games list.");
	}

	auto json_ret = requestResult.data.ReadString();
	auto parsed_json = nlohmann::json::parse(json_ret);

	auto gamesList = PopulateGameList(parsed_json);
	if(gamesList.empty())
	{
		throw std::runtime_error("Failed to get game.");
	}

	return gamesList;
}

GamesList CClient::PopulateGameList(const nlohmann::json& parsed_json)
{
	GamesList list;

	if(parsed_json["data"]["count"].get<int>() == 0)
	{
		return list;
	}

	auto games = parsed_json["data"]["games"].get<std::vector<nlohmann::json>>();

	std::string image_base = "";
	auto includes = parsed_json["include"];
	if(!includes.empty())
	{
		image_base = includes["boxart"]["base_url"]["medium"].get<std::string>();
	}
	for(auto game : games)
	{
		int game_id = game["id"].get<int>();
		TheGamesDb::Game meta;
		meta.id = game_id;
		meta.overview = game["overview"].get<std::string>();
		if(!game["uids"].empty())
		{
			auto uids = game["uids"].get<std::vector<nlohmann::json>>();
			for(auto item : uids)
			{
				meta.discIds.push_back(item["uid"].get<std::string>());
			}
		}
		meta.title = game["game_title"].get<std::string>();
		meta.baseImgUrl = image_base;
		if(!includes.empty())
		{
			auto boxarts = includes["boxart"]["data"];
			if(!boxarts.empty())
			{
				auto str_id = std::to_string(game_id);
				auto games_cover_meta = boxarts[str_id.c_str()];
				if(!games_cover_meta.empty())
				{
					for(auto& game_cover : games_cover_meta)
					{

						meta.boxArtUrl = game_cover["filename"].get<std::string>().c_str();
						if(game_cover["side"].get<std::string>() == "front")
						{
							break;
						}
					}
				}
			}
		}
		list.push_back(meta);
	}

	return list;
}
