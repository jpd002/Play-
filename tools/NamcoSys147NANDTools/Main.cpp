#include "iop/namco_sys147/NamcoSys147NANDReader.h"
#include "StdStreamUtils.h"
#include "filesystem_def.h"
#include "PathUtils.h"

void ExtractDir(Namco::CSys147NANDReader& reader, uint32 dirSector, fs::path& outputDir)
{
	Framework::PathUtils::EnsurePathExists(outputDir);
	auto dir = reader.ReadDirectory(dirSector);
	for(const auto& dirEntry : dir)
	{
		uint32 absSector = dirSector + dirEntry.sector;
		auto entryOutputPath = outputDir / dirEntry.name;
		if(dirEntry.type == 'D')
		{
			printf("Entering dir %s.\n", dirEntry.name);
			auto subOutputDir = outputDir / dirEntry.name;
			ExtractDir(reader, absSector, entryOutputPath);
			printf("Leaving dir %s.\n", dirEntry.name);
		}
		else
		{
			printf("Extracting %s.\n", dirEntry.name);
			auto fileContents = reader.ReadFile(absSector, dirEntry.size);
			auto outputFile = outputDir / dirEntry.name;
			auto outputStream = Framework::CreateOutputStdStream(entryOutputPath.native());
			outputStream.Write(fileContents.data(), fileContents.size());
		}
	}
}

int main(int argc, char** argv)
{
	if(argc < 4)
	{
		printf("NamcoSys147NANDTools <nand path> <root sector> <output dir>\n");
		return -1;
	}
	auto nandPath = fs::path(argv[1]);
	int32 rootSector = strtol(argv[2], nullptr, 0);
	auto outputDir = fs::path(argv[3]);
	auto inputStream = Framework::CreateInputStdStream(nandPath.native());
	Namco::CSys147NANDReader reader(inputStream, rootSector);
	ExtractDir(reader, 0, outputDir);
	return 0;
}
