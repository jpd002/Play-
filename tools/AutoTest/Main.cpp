#include "PS2VM.h"
#include <boost/filesystem.hpp>
#include "StdStream.h"

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
			ExecuteTest(testPath);
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
