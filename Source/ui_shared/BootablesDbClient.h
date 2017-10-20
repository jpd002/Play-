#pragma once

#include <string>
#include <boost/filesystem.hpp>
#include "Types.h"
#include "Singleton.h"
#include "sqlite/SqliteDb.h"

namespace BootablesDb
{
	struct Bootable
	{
		boost::filesystem::path path;
		std::string discId;
		time_t lastBootedTime = 0;
	};

	class CClient : public CSingleton<CClient>
	{
	public:
		CClient();
		virtual ~CClient() = default;

		Bootable GetBootable(const boost::filesystem::path&);
		std::vector<Bootable> GetBootables();

		void RegisterBootable(const boost::filesystem::path&);

		void SetDiscId(const boost::filesystem::path&, const char*);
		void SetLastBootedTime(const boost::filesystem::path&, time_t);

	private:
		void CheckDbVersion();

		boost::filesystem::path m_dbPath;
		Framework::CSqliteDb m_db;
	};
};
