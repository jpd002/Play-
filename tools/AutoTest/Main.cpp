#include "PS2VM.h"
#include <boost/filesystem.hpp>
#include "StdStream.h"
#include "StdStreamUtils.h"
#include "Utils.h"
#include "JUnitTestReportWriter.h"

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

TESTRESULT GetTestResult(const boost::filesystem::path& testFilePath)
{
	TESTRESULT result;
	result.succeeded = false;
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

		if(resultLines.size() != expectedLines.size()) return result;

		for(unsigned int i = 0; i < resultLines.size(); i++)
		{
			if(resultLines[i] != expectedLines[i])
			{
				LINEDIFF lineDiff;
				lineDiff.expected = expectedLines[i];
				lineDiff.result = resultLines[i];
				result.lineDiffs.push_back(lineDiff);
			}
		}

		result.succeeded = result.lineDiffs.empty();
		return result;
	}
	catch(...)
	{

	}
	return result;
}

void ExecuteEeTest(const boost::filesystem::path& testFilePath)
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
	virtualMachine.m_iopOs->GetIoman()->SetFileStream(Iop::CIoman::FID_STDOUT, resultStream);
	virtualMachine.Resume();

	while(!executionOver)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	virtualMachine.Pause();
	virtualMachine.Destroy();
}

void ExecuteIopTest(const boost::filesystem::path& testFilePath)
{
	//Read in the module data
	std::vector<uint8> moduleData;
	{
		auto moduleStream = Framework::CreateInputStdStream(testFilePath.native());
		auto length = moduleStream.GetLength();
		moduleData.resize(length);
		moduleStream.Read(moduleData.data(), length);
	}

	auto resultFilePath = testFilePath;
	resultFilePath.replace_extension(".result");
	auto resultStream = new Framework::CStdStream(resultFilePath.string().c_str(), "wb");

	bool executionOver = false;

	//Setup virtual machine
	CPS2VM virtualMachine;
	virtualMachine.Initialize();
	virtualMachine.Reset();
	int32 rootModuleId = virtualMachine.m_iopOs->LoadModuleFromHost(moduleData.data());
	virtualMachine.m_iopOs->OnModuleStarted.connect(
		[&executionOver, &rootModuleId] (uint32 moduleId)
		{
			if(rootModuleId == moduleId)
			{
				executionOver = true;
			}
		}
	);
	virtualMachine.m_iopOs->StartModule(rootModuleId, "", nullptr, 0);
	virtualMachine.m_iopOs->GetIoman()->SetFileStream(Iop::CIoman::FID_STDOUT, resultStream);
	virtualMachine.Resume();

	while(!executionOver)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	virtualMachine.Pause();
	virtualMachine.Destroy();
}

void ScanAndExecuteTests(const boost::filesystem::path& testDirPath, const TestReportWriterPtr& testReportWriter)
{
	boost::filesystem::directory_iterator endIterator;
	for(auto testPathIterator = boost::filesystem::directory_iterator(testDirPath);
		testPathIterator != endIterator; testPathIterator++)
	{
		auto testPath = testPathIterator->path();
		if(boost::filesystem::is_directory(testPath))
		{
			ScanAndExecuteTests(testPath, testReportWriter);
			continue;
		}
		if(testPath.extension() == ".elf")
		{
			printf("Testing '%s': ", testPath.string().c_str());
			ExecuteEeTest(testPath);
			auto result = GetTestResult(testPath);
			printf("%s.\r\n", result.succeeded ? "SUCCEEDED" : "FAILED");
			if(testReportWriter)
			{
				testReportWriter->ReportTestEntry(testPath.string(), result);
			}
		}
		else if(testPath.extension() == ".irx")
		{
			printf("Testing '%s': ", testPath.string().c_str());
			ExecuteIopTest(testPath);
			auto result = GetTestResult(testPath);
			printf("%s.\r\n", result.succeeded ? "SUCCEEDED" : "FAILED");
			if(testReportWriter)
			{
				testReportWriter->ReportTestEntry(testPath.string(), result);
			}
		}
	}
}

int main(int argc, const char** argv)
{
	if(argc < 2)
	{
		printf("Usage: AutoTest [options] testDir\r\n");
		printf("Options: \r\n");
		printf("\t --junitreport <path>\t Writes JUnit format report at <path>.\r\n");
		return -1;
	}

	TestReportWriterPtr testReportWriter;
	boost::filesystem::path autoTestRoot;
	boost::filesystem::path reportPath;

	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "--junitreport"))
		{
			if((i + 1) >= argc)
			{
				printf("Error: Path must be specified for --junitreport option.\r\n");
				return -1;
			}
			testReportWriter = std::make_shared<CJUnitTestReportWriter>();
			reportPath = boost::filesystem::path(argv[i + 1]);
			i++;
		}
		else
		{
			autoTestRoot = argv[i];
			break;
		}
	}

	if(autoTestRoot.empty())
	{
		printf("Error: No test directory specified.\r\n");
		return -1;
	}

	ScanAndExecuteTests(autoTestRoot, testReportWriter);

	if(testReportWriter)
	{
		try
		{
			testReportWriter->Write(reportPath);
		}
		catch(const std::exception& exception)
		{
			printf("Error: Failed to write test report: %s\r\n", exception.what());
			return -1;
		}
	}

	return 0;
}
