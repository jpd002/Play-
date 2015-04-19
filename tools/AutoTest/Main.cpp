#include "PS2VM.h"
#include <boost/filesystem.hpp>
#include "StdStream.h"
#include "StdStreamUtils.h"
#include "Utils.h"

std::vector<std::string> ReadLines(Framework::CStream& inputStream)
{
	std::vector<std::string> lines;
	lines.push_back(Utils::GetLine(&inputStream));
	while(!inputStream.IsEOF())
	{
		lines.push_back(Utils::GetLine(&inputStream));
	}
	return lines;
}

bool CompareResults(const boost::filesystem::path& testFilePath)
{
	try
	{
		auto resultFilePath = testFilePath;
		resultFilePath.replace_extension(".result");
		auto expectedFilePath = testFilePath;
		expectedFilePath.replace_extension(".expected");
		auto resultStream = Framework::CreateInputStdStream(resultFilePath.string());
		auto expectedStream = Framework::CreateInputStdStream(expectedFilePath.string());

		auto resultLines = ReadLines(resultStream);
		auto expectedLines = ReadLines(expectedStream);

		if(resultLines.size() != expectedLines.size()) return false;

		for(unsigned int i = 0; i < resultLines.size(); i++)
		{
			if(resultLines[i] != expectedLines[i]) return false;
		}

		return true;
	}
	catch(...)
	{
		return false;
	}
}

void ExecuteTest(const boost::filesystem::path& testFilePath)
{
	auto resultFilePath = testFilePath;
	resultFilePath.replace_extension(".result");
	auto resultStream = new Framework::CStdStream(resultFilePath.string().c_str(), "wb");

	bool executionOver = false;

	//Setup virtual machine
	CPS2VM virtualMachine;
	virtualMachine.Initialize();
	virtualMachine.Reset();
	virtualMachine.m_ee->m_os->OnRequestExit.connect(
		[&executionOver] ()
		{
			executionOver = true;
		}
	);
	virtualMachine.m_ee->m_os->BootFromFile(testFilePath.string().c_str());
	virtualMachine.m_iopOs->GetIoman()->SetFileStream(1, resultStream);
	virtualMachine.Resume();

	while(!executionOver)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	virtualMachine.Pause();
	virtualMachine.Destroy();
}

void ScanAndExecuteTests(const boost::filesystem::path& testDirPath)
{
	boost::filesystem::directory_iterator endIterator;
	for(auto testPathIterator = boost::filesystem::directory_iterator(testDirPath);
		testPathIterator != endIterator; testPathIterator++)
	{
		auto testPath = testPathIterator->path();
		if(boost::filesystem::is_directory(testPath))
		{
			ScanAndExecuteTests(testPath);
			continue;
		}
		if(testPath.extension() == ".elf")
		{
			printf("Testing '%s': ", testPath.string().c_str());
			ExecuteTest(testPath);
			bool succeeded = CompareResults(testPath);
			printf("%s.\r\n", succeeded ? "SUCCEEDED" : "FAILED");
		}
	}
}

int main(int argc, const char** argv)
{
	if(argc < 2)
	{
		printf("Usage: AutoTest [test directory]\r\n");
		return -1;
	}
	boost::filesystem::path autoTestRoot = argv[1];
	ScanAndExecuteTests(autoTestRoot);
	return 0;
}
