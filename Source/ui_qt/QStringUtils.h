#pragma once

#include <QString>
#include "boost_filesystem_def.h"

boost::filesystem::path QStringToPath(const QString&);
QString PathToQString(const boost::filesystem::path&);
