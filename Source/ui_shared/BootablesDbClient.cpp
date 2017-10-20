#include <cassert>
#include "../AppConfig.h"
#include "string_format.h"
#include "PathUtils.h"
#include "BootablesDbClient.h"
#include "sqlite/SqliteStatement.h"

using namespace BootablesDb;

#define DATABASE_VERSION 2

static const char* g_dbFileName = "bootables.db";

static const char* g_bootablesTableCreateStatement = 
"CREATE TABLE IF NOT EXISTS bootables"
"("
"    path TEXT PRIMARY KEY,"
"    discId VARCHAR(10) DEFAULT '',"
"    title TEXT DEFAULT '',"
"    coverUrl TEXT DEFAULT '',"
"    lastBootedTime INTEGER DEFAULT 0"
")";

CClient::CClient()
{
	m_dbPath = CAppConfig::GetInstance().GetBasePath() / g_dbFileName;

	CheckDbVersion();

	m_db = Framework::CSqliteDb(Framework::PathUtils::GetNativeStringFromPath(m_dbPath).c_str(),
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

	{
		auto query = string_format("PRAGMA user_version = %d", DATABASE_VERSION);
		Framework::CSqliteStatement statement(m_db, query.c_str());
		statement.StepNoResult();
	}

	{
		Framework::CSqliteStatement statement(m_db, g_bootablesTableCreateStatement);
		statement.StepNoResult();
	}
}

Bootable CClient::GetBootable(const boost::filesystem::path& path)
{
	Framework::CSqliteStatement statement(m_db, "SELECT * FROM bootables WHERE path = ?");
	statement.BindText(1, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepWithResult();
	return ReadBootable(statement);
}

std::vector<Bootable> CClient::GetBootables()
{
	std::vector<Bootable> bootables;

	Framework::CSqliteStatement statement(m_db, "SELECT * FROM bootables");
	while(statement.Step())
	{
		auto bootable = ReadBootable(statement);
		bootables.push_back(bootable);
	}

	return bootables;
}

void CClient::RegisterBootable(const boost::filesystem::path& path)
{
	Framework::CSqliteStatement statement(m_db, "INSERT OR IGNORE INTO bootables (path) VALUES (?)");
	statement.BindText(1, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

void CClient::SetDiscId(const boost::filesystem::path& path, const char* discId)
{
	Framework::CSqliteStatement statement(m_db, "UPDATE bootables SET discId = ? WHERE path = ?");
	statement.BindText(1, discId, true);
	statement.BindText(2, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

void CClient::SetTitle(const boost::filesystem::path& path, const char* title)
{
	Framework::CSqliteStatement statement(m_db, "UPDATE bootables SET title = ? WHERE path = ?");
	statement.BindText(1, title, true);
	statement.BindText(2, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

void CClient::SetLastBootedTime(const boost::filesystem::path& path, time_t lastBootedTime)
{
	Framework::CSqliteStatement statement(m_db, "UPDATE bootables SET lastBootedTime = ? WHERE path = ?");
	statement.BindInteger(1, lastBootedTime);
	statement.BindText(2, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

Bootable CClient::ReadBootable(Framework::CSqliteStatement& statement)
{
	Bootable bootable;
	bootable.path           = Framework::PathUtils::GetPathFromNativeString(reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)));
	bootable.discId         = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
	bootable.title          = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
	bootable.coverUrl       = reinterpret_cast<const char*>(sqlite3_column_text(statement, 3));
	bootable.lastBootedTime = sqlite3_column_int(statement, 4);
	return bootable;
}

void CClient::CheckDbVersion()
{
	bool dbExistsAndMatchesVersion = 
		[&] ()
		{
			try
			{
				auto db = Framework::CSqliteDb(Framework::PathUtils::GetNativeStringFromPath(m_dbPath).c_str(),
					SQLITE_OPEN_READONLY);

				Framework::CSqliteStatement statement(db, "PRAGMA user_version");
				statement.StepWithResult();
				int version = sqlite3_column_int(statement, 0);
				return (version == DATABASE_VERSION);
			}
			catch(...)
			{
				return false;
			}
		} ();

	if(!dbExistsAndMatchesVersion)
	{
		boost::filesystem::remove(m_dbPath);
	}
}
