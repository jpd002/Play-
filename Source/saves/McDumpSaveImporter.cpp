#include "McDumpSaveImporter.h"
#include "StdStreamUtils.h"

void CMcDumpSaveImporter::Import(Framework::CStream& input, const fs::path& basePath)
{
	CMcDumpReader reader(input);
	auto dir = reader.ReadRootDirectory();
	ImportDirectory(reader, dir, basePath);
}

void CMcDumpSaveImporter::ImportDirectory(CMcDumpReader& reader, const CMcDumpReader::Directory& dir, const fs::path& basePath)
{
	if(!fs::exists(basePath))
	{
		fs::create_directory(basePath);
	}

	for(const auto& dirEntry : dir)
	{
		if(!strcmp(dirEntry.name, ".")) continue;
		if(!strcmp(dirEntry.name, "..")) continue;
		auto entryOutputPath = basePath / dirEntry.name;
		if(dirEntry.mode & CMcDumpReader::DF_DIRECTORY)
		{
			auto subDir = reader.ReadDirectory(dirEntry.cluster, dirEntry.length);
			ImportDirectory(reader, subDir, entryOutputPath);
		}
		else
		{
			if(CanExtractFile(entryOutputPath))
			{
				auto fileContents = reader.ReadFile(dirEntry.cluster, dirEntry.length);
				assert(fileContents.size() == dirEntry.length);
				auto output = Framework::CreateOutputStdStream(entryOutputPath.native());
				output.Write(fileContents.data(), fileContents.size());
			}
		}
	}
}
