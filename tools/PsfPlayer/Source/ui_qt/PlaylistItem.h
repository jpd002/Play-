#pragma once
#include <string>

class Playlist
{
public:
	struct Item
	{
		Item(std::wstring m_game, std::wstring m_title, std::wstring m_length, std::string m_path)
		{
			game = m_game;
			title = m_title;
			length = m_length;
			path = m_path;
		}

		~Item()
		{
		}

		std::wstring game, title, length;
		std::string path;
	};
};
