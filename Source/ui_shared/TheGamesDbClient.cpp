#include <memory>
#include <cstdlib>
#include <cstring>
#include "TheGamesDbClient.h"
#include "string_format.h"
#include "http/HttpClientFactory.h"
#include "xml/Node.h"
#include "xml/Parser.h"

using namespace TheGamesDb;

static const char* g_getGameUrl      = "http://thegamesdb.net/api/GetGame.php?id=%d";
static const char* g_getGamesListUrl = "http://thegamesdb.net/api/GetGamesList.php?platform=%s&name=%s";

bool TryGetNodeValue(Framework::Xml::CNode* document, const char* nodePath, std::string& value)
{
	auto node = document->Select(nodePath);
	if(!node) return false;
	auto innerText = node->GetInnerText();
	if(!innerText) return false;
	value = innerText;
	return true;
}

Game CClient::GetGame(uint32 id)
{
	auto url = string_format(g_getGameUrl, id);
	auto requestResult = 
		[&] ()
		{
			auto client = Framework::Http::CreateHttpClient();
			client->SetUrl(url);
			return client->SendRequest();
		}();

	if(requestResult.statusCode != Framework::Http::HTTP_STATUS_CODE::OK)
	{
		throw std::runtime_error("Failed to get game.");
	}

	auto document = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(requestResult.data));

	//Check if there's an error
	{
		auto errorNode = document->Select("Error");
		if(errorNode)
		{
			throw std::runtime_error("Failed to get game.");
		}
	}

	Game game;

	TryGetNodeValue(document.get(), "Data/baseImgUrl", game.baseImgUrl);
	TryGetNodeValue(document.get(), "Data/Game/GameTitle", game.title);
	TryGetNodeValue(document.get(), "Data/Game/Overview", game.overview);

	{
		auto imagesNode = document->Select("Data/Game/Images");
		if(imagesNode)
		{
			for(const auto& imageNode : imagesNode->GetChildren())
			{
				if(strcmp(imageNode->GetText(), "boxart")) continue;

				auto boxArtSide = imageNode->GetAttribute("side");
				auto boxArtUrl = imageNode->GetInnerText();
				if(!boxArtSide || !boxArtUrl) continue;
				if(strcmp(boxArtSide, "front")) continue;

				game.boxArtUrl = boxArtUrl;
			}
		}
	}

	return game;
}

GamesList CClient::GetGamesList(const std::string& platformName, const std::string& name)
{
	auto encodedPlatformName = Framework::Http::CHttpClient::UrlEncode(platformName);
	auto encodedName = Framework::Http::CHttpClient::UrlEncode(name);

	auto url = string_format(g_getGamesListUrl, encodedPlatformName.c_str(), encodedName.c_str());
	auto requestResult = 
		[&] ()
		{
			auto client = Framework::Http::CreateHttpClient();
			client->SetUrl(url);
			return client->SendRequest();
		}();

	if(requestResult.statusCode != Framework::Http::HTTP_STATUS_CODE::OK)
	{
		throw std::runtime_error("Failed to get games list.");
	}

	auto document = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(requestResult.data));

	//Check if there's an error
	{
		auto errorNode = document->Select("Error");
		if(errorNode)
		{
			throw std::runtime_error("Failed to get games list.");
		}
	}

	GamesList gamesList;

	auto gameNodes = document->SelectNodes("Data/Game");
	for(auto gameNode : gameNodes)
	{
		auto idNode = gameNode->Select("id");
		auto gameTitleNode = gameNode->Select("GameTitle");
		if(!idNode || !gameTitleNode) continue;
		auto idString = idNode->GetInnerText();
		auto gameTitleString = gameTitleNode->GetInnerText();
		if(!idString || !gameTitleString) continue;
		GamesListItem gamesListItem;
		gamesListItem.id        = atol(idString);
		gamesListItem.gameTitle = gameTitleString;
		gamesList.push_back(gamesListItem);
	}

	return gamesList;
}
