#pragma once

#include <map>
#include <mutex>
#include <string>
#include <QPixmap>
#include "ui_shared/BootablesDbClient.h"

class CoverUtils
{

private:
	static std::map<std::string, QPixmap> cache;
	static std::mutex m_lock;

public:
	static void PopulateCache(std::vector<BootablesDb::Bootable>);
	static QPixmap find(std::string key);
	static void PopulatePlaceholderCover();
};
