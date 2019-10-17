#pragma once

#include <QString>
#include "filesystem_def.h"

fs::path QStringToPath(const QString&);
QString PathToQString(const fs::path&);
