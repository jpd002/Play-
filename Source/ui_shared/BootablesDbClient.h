#pragma once

#include <string>
#include <boost/filesystem.hpp>
#include "Types.h"
#include "Singleton.h"
#include "sqlite/SqliteDb.h"
#include "sqlite/SqliteStatement.h"

namespace BootablesDb
{
	struct Bootable
	{
		boost::filesystem::path path;
		std::string discId;
		std::string title;
		std::string coverUrl;
		std::string overview;
		time_t lastBootedTime = 0;
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

		Bootable GetBootable(const boost::filesystem::path&);
		std::vector<Bootable> GetBootables(int32_t = SORT_METHOD_NONE);

		void RegisterBootable(const boost::filesystem::path&, const char*, const char*);
		void UnregisterBootable(const boost::filesystem::path&);

		void SetDiscId(const boost::filesystem::path&, const char*);
		void SetTitle(const boost::filesystem::path&, const char*);
		void SetCoverUrl(const boost::filesystem::path&, const char*);
		void SetLastBootedTime(const boost::filesystem::path&, time_t);
		void SetOverview(const boost::filesystem::path& path, const char* overview);

	private:
		static Bootable ReadBootable(Framework::CSqliteStatement&);

		void CheckDbVersion();

		boost::filesystem::path m_dbPath;
		Framework::CSqliteDb m_db;
	};
};
