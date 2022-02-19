#pragma once

#include <string>
#include <vector>
#include "filesystem_def.h"
#include "Types.h"
#include "Singleton.h"
#include "sqlite/SqliteDb.h"
#include "sqlite/SqliteStatement.h"

namespace BootablesDb
{
	struct BootableState
	{
		std::string name;
		std::string color;
	};
	using BootableStateList = std::vector<BootableState>;

	struct Bootable
	{
		fs::path path;
		std::string discId;
		std::string title;
		std::string coverUrl;
		std::string overview;
		time_t lastBootedTime = 0;
		BootableStateList states;
	};

	class CClient : public CSingleton<CClient>
	{
	public:
		//NOTE: This is duplicated in the Android Java code (in BootablesInterop.java) - values matter
		enum SORT_METHOD
		{
			SORT_METHOD_RECENT,
			SORT_METHOD_HOMEBREW,
			SORT_METHOD_NONE,
		};

		CClient();
		virtual ~CClient() = default;

		bool BootableExists(const fs::path&);
		Bootable GetBootable(const fs::path&);
		std::vector<Bootable> GetBootables(int32_t = SORT_METHOD_NONE);
		BootableStateList GetStates();

		void RegisterBootable(const fs::path&, const char*, const char*);
		void UnregisterBootable(const fs::path&);

		void SetDiscId(const fs::path&, const char*);
		void SetTitle(const fs::path&, const char*);
		void SetCoverUrl(const fs::path&, const char*);
		void SetLastBootedTime(const fs::path&, time_t);
		void SetOverview(const fs::path& path, const char* overview);

	private:
		Bootable ReadBootable(Framework::CSqliteStatement&);
		BootableStateList GetGameStates(std::string);

		void CheckDbVersion();

		fs::path m_dbPath;
		Framework::CSqliteDb m_db;
		bool m_attachedState = false;
	};
};
