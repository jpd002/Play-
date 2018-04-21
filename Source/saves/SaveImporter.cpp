#include "SaveImporter.h"
#include "MaxSaveImporter.h"
#include "PsuSaveImporter.h"
#include "StdStreamUtils.h"
#include "XpsSaveImporter.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace filesystem = boost::filesystem;

void CSaveImporter::ImportSave(Framework::CStream& input, const boost::filesystem::path& outputPath, const OverwritePromptHandlerType& overwritePromptHandler)
{
	std::shared_ptr<CSaveImporterBase> importer;

	uint32 signature = input.Read32();
	input.Seek(0, Framework::STREAM_SEEK_SET);

	if(signature == 0x00008427)
	{
		importer = std::make_shared<CPsuSaveImporter>();
	}
	else if(signature == 0x0000000D)
	{
		importer = std::make_shared<CXpsSaveImporter>();
	}
	else if(signature == 0x50327350)
	{
		importer = std::make_shared<CMaxSaveImporter>();
	}
	else
	{
		throw std::runtime_error("Unknown input file format.");
	}

	importer->SetOverwritePromptHandler(overwritePromptHandler);
	importer->Import(input, outputPath);
}
