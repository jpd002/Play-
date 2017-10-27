#pragma once

#include <boost/filesystem.hpp>

void ScanBootables(const boost::filesystem::path&);
void PurgeInexistingFiles();
void ExtractDiscIds();
void FetchGameTitles();
void FetchCoverUrls();
