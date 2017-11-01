#pragma once

#include <boost/filesystem.hpp>

bool IsBootableExecutablePath(const boost::filesystem::path&);
bool IsBootableDiscImagePath(const boost::filesystem::path&);
void ScanBootables(const boost::filesystem::path&);
void PurgeInexistingFiles();
void ExtractDiscIds();
void FetchGameTitles();
void FetchCoverUrls();
