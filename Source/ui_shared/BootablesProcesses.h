#pragma once

#include <boost/filesystem.hpp>

void ScanBootables(const boost::filesystem::path&);
void ExtractDiscIds();
void FetchGameTitles();
