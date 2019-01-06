#pragma once

#include <boost/filesystem.hpp>
#include <set>

bool IsBootableExecutablePath(const boost::filesystem::path&);
bool IsBootableDiscImagePath(const boost::filesystem::path&);
void ScanBootables(const boost::filesystem::path&, bool = true);
std::set<boost::filesystem::path> GetActiveBootableDirectories();
void PurgeInexistingFiles();
void FetchGameTitles();
