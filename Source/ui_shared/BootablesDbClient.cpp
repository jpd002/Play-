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
    "    lastBootedTime INTEGER DEFAULT 0,"
    "    overview TEXT DEFAULT ''"
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

	{
		auto path = Framework::PathUtils::GetAppResourcesPath() / "states.db";
		std::error_code errorCode;
		if(fs::exists(path, errorCode))
		{
			char* err = NULL;
			auto query = string_format("ATTACH DATABASE '%s' as 'stateDB';", path.string().c_str());
			sqlite3_exec(m_db, query.c_str(), 0, 0, &err);
			m_attachedState = err == NULL;
		}
	}
}

bool CClient::BootableExists(const fs::path& path)
{
	Framework::CSqliteStatement statement(m_db, "SELECT * FROM bootables WHERE path = ?");
	statement.BindText(1, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	return statement.Step();
}

Bootable CClient::GetBootable(const fs::path& path)
{
	Framework::CSqliteStatement statement(m_db, "SELECT * FROM bootables WHERE path = ?");
	statement.BindText(1, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepWithResult();
	return ReadBootable(statement);
}

std::vector<Bootable> CClient::GetBootables(int32_t sortMethod)
{
	std::string query = "SELECT * FROM bootables ";

	switch(sortMethod)
	{
	case SORT_METHOD_RECENT:
		query += "WHERE lastBootedTime != 0 Order By lastBootedTime DESC ";
		break;
	case SORT_METHOD_HOMEBREW:
		query += "WHERE path LIKE '%.elf' COLLATE NOCASE ";
		break;
	case SORT_METHOD_NONE:
		break;
	}
	std::vector<Bootable> bootables;

	Framework::CSqliteStatement statement(m_db, query.c_str());
	while(statement.Step())
	{
		auto bootable = ReadBootable(statement);
		bootables.push_back(bootable);
	}

	return bootables;
}

void CClient::RegisterBootable(const fs::path& path, const char* title, const char* discId)
{
	Framework::CSqliteStatement statement(m_db, "INSERT OR IGNORE INTO bootables (path, title, discId) VALUES (?,?,?)");
	statement.BindText(1, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.BindText(2, title, true);
	statement.BindText(3, discId, true);
	statement.StepNoResult();
}

void CClient::UnregisterBootable(const fs::path& path)
{
	Framework::CSqliteStatement statement(m_db, "DELETE FROM bootables WHERE path = ?");
	statement.BindText(1, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

void CClient::SetDiscId(const fs::path& path, const char* discId)
{
	Framework::CSqliteStatement statement(m_db, "UPDATE bootables SET discId = ? WHERE path = ?");
	statement.BindText(1, discId, true);
	statement.BindText(2, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

void CClient::SetTitle(const fs::path& path, const char* title)
{
	Framework::CSqliteStatement statement(m_db, "UPDATE bootables SET title = ? WHERE path = ?");
	statement.BindText(1, title, true);
	statement.BindText(2, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

void CClient::SetCoverUrl(const fs::path& path, const char* coverUrl)
{
	Framework::CSqliteStatement statement(m_db, "UPDATE bootables SET coverUrl = ? WHERE path = ?");
	statement.BindText(1, coverUrl, true);
	statement.BindText(2, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

void CClient::SetLastBootedTime(const fs::path& path, time_t lastBootedTime)
{
	Framework::CSqliteStatement statement(m_db, "UPDATE bootables SET lastBootedTime = ? WHERE path = ?");
	statement.BindInteger(1, lastBootedTime);
	statement.BindText(2, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

void CClient::SetOverview(const fs::path& path, const char* overview)
{
	Framework::CSqliteStatement statement(m_db, "UPDATE bootables SET overview = ? WHERE path = ?");
	statement.BindText(1, overview, true);
	statement.BindText(2, Framework::PathUtils::GetNativeStringFromPath(path).c_str());
	statement.StepNoResult();
}

BootableStateList CClient::GetGameStates(std::string discId)
{
	BootableStateList states;
	if(!m_attachedState)
		return states;

	std::string query = "SELECT stateDB.labels.name AS stateName, stateDB.labels.color AS stateColor FROM stateDB.games ";
	query += "LEFT JOIN stateDB.labels ON stateDB.games.state = stateDB.labels.name ";
	query += "WHERE stateDB.games.discId = ?;";

	Framework::CSqliteStatement statement(m_db, query.c_str());
	statement.BindText(1, discId.c_str(), true);
	while(statement.Step())
	{
		BootableState state;
		state.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
		state.color = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
		states.push_back(state);
	}
	return states;
}

BootableStateList CClient::GetStates()
{
	BootableStateList states;
	if(!m_attachedState)
		return states;

	std::string query = "SELECT stateDB.labels.name AS stateName, stateDB.labels.color AS stateColor FROM stateDB.labels;";

	Framework::CSqliteStatement statement(m_db, query.c_str());
	while(statement.Step())
	{
		BootableState state;
		state.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
		state.color = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
		states.push_back(state);
	}
	return states;
}

Bootable CClient::ReadBootable(Framework::CSqliteStatement& statement)
{
	Bootable bootable;
	bootable.path = Framework::PathUtils::GetPathFromNativeString(reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)));
	bootable.discId = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
	bootable.title = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
	bootable.coverUrl = reinterpret_cast<const char*>(sqlite3_column_text(statement, 3));
	bootable.overview = reinterpret_cast<const char*>(sqlite3_column_text(statement, 5));
	bootable.lastBootedTime = sqlite3_column_int(statement, 4);
	bootable.states = GetGameStates(bootable.discId);
	return bootable;
}

void CClient::CheckDbVersion()
{
	bool dbExistsAndMatchesVersion =
	    [&]() {
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
	    }();

	if(!dbExistsAndMatchesVersion)
	{
		fs::remove(m_dbPath);
	}
}
