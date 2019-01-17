#include <iostream>

#include "AppConfig.h"
#include "CoverUtils.h"
#include "PathUtils.h"
#include "ui_shared/BootablesDbClient.h"
#include "QStringUtils.h"

std::map<std::string, QPixmap> CoverUtils::cache;

QPixmap CoverUtils::find(std::string key)
{
	auto itr = CoverUtils::cache.find(key);
	if(itr == CoverUtils::cache.end())
	{
		itr = CoverUtils::cache.find("PH");
	}
	return itr->second;
}

void CoverUtils::PopulateCache()
{
	auto coverpath(CAppConfig::GetBasePath() / boost::filesystem::path("covers"));
	Framework::PathUtils::EnsurePathExists(coverpath);

	auto bootables = BootablesDb::CClient::GetInstance().GetBootables();
	for(auto bootable : bootables)
	{
		if(bootable.discId.empty())
			continue;

		auto path = coverpath / (bootable.discId + ".jpg");
		if(boost::filesystem::exists(path))
		{
			auto itr = CoverUtils::cache.find(bootable.discId.c_str());
			if(itr == CoverUtils::cache.end())
			{
				auto pixmap = QPixmap(PathToQString(path));
				pixmap = pixmap.scaledToWidth(250 / 2, Qt::SmoothTransformation);
				CoverUtils::cache.insert(std::make_pair(bootable.discId.c_str(), pixmap));
			}
		}
	}

	auto itr = CoverUtils::cache.find("PH");
	if(itr == CoverUtils::cache.end())
	{
		auto pixmap = QPixmap(QString(":/assets/boxart.png")).scaledToWidth(250 / 2, Qt::SmoothTransformation);
		CoverUtils::cache.insert(std::make_pair("PH", pixmap));
	}
}
