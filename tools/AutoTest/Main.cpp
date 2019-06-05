#include "PS2VM.h"
#include <boost/filesystem.hpp>
#include "StdStream.h"
#include "StdStreamUtils.h"
#include "iop/IopBios.h"
#include "JUnitTestReportWriter.h"
#include "gs/GSH_Null.h"
#ifdef _WIN32
#include "gs/GSH_OpenGLWin32/GSH_OpenGLWin32.h"
#include "gs/GSH_Direct3D9/GSH_Direct3D9.h"
#endif

#define GS_HANDLER_NAME_NULL "null"
#define GS_HANDLER_NAME_OGL "ogl"
#define GS_HANDLER_NAME_D3D9 "d3d9"

#define DEFAULT_GS_HANDLER_NAME GS_HANDLER_NAME_NULL

static std::set<std::string> g_validGsHandlersNames =
    {
        GS_HANDLER_NAME_NULL,
#ifdef _WIN32
        GS_HANDLER_NAME_OGL,
        GS_HANDLER_NAME_D3D9,
#endif
};

#ifdef _WIN32

class CTestWindow : public Framework::Win32::CWindow, public CSingleton<CTestWindow>
{
public:
	CTestWindow()
	{
		Create(0, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WS_OVERLAPPED, Framework::Win32::CRect(0, 0, 100, 100), NULL, NULL);
		SetClassPtr();
	}
};

#endif

CGSHandler::FactoryFunction GetGsHandlerFactoryFunction(const std::string& gsHandlerName)
{
	if(gsHandlerName == GS_HANDLER_NAME_NULL)
	{
		return CGSH_Null::GetFactoryFunction();
	}
#ifdef _WIN32
	else if(gsHandlerName == GS_HANDLER_NAME_OGL)
	{
		return CGSH_OpenGLWin32::GetFactoryFunction(&CTestWindow::GetInstance());
	}
	else if(gsHandlerName == GS_HANDLER_NAME_D3D9)
	{
		return CGSH_Direct3D9::GetFactoryFunction(&CTestWindow::GetInstance());
	}
#endif
	else
	{
		throw std::runtime_error("Unknown GS handler name.");
	}
}

std::vector<std::string> ReadLines(Framework::CStream& inputStream)
{
	std::vector<std::string> lines;
	lines.push_back(inputStream.ReadLine());
	while(!inputStream.IsEOF())
	{
		lines.push_back(inputStream.ReadLine());
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

void ExecuteEeTest(const boost::filesystem::path& testFilePath, const std::string& gsHandlerName)
{
	auto resultFilePath = testFilePath;
	resultFilePath.replace_extension(".result");
	auto resultStream = new Framework::CStdStream(resultFilePath.string().c_str(), "wb");

	bool executionOver = false;

	//Setup virtual machine
	CPS2VM virtualMachine;
	virtualMachine.Initialize();
	virtualMachine.Reset();
	virtualMachine.CreateGSHandler(GetGsHandlerFactoryFunction(gsHandlerName));
	virtualMachine.m_ee->m_os->OnRequestExit.connect(
	    [&executionOver]() {
		    executionOver = true;
	    });
	virtualMachine.m_ee->m_os->BootFromFile(testFilePath);
	{
		auto iopOs = dynamic_cast<CIopBios*>(virtualMachine.m_iop->m_bios.get());
		iopOs->GetIoman()->SetFileStream(Iop::CIoman::FID_STDOUT, resultStream);
	}
	virtualMachine.Resume();

	while(!executionOver)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	virtualMachine.Pause();
	virtualMachine.DestroyGSHandler();
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
	{
		auto iopOs = dynamic_cast<CIopBios*>(virtualMachine.m_iop->m_bios.get());
		int32 rootModuleId = iopOs->LoadModuleFromHost(moduleData.data());
		iopOs->OnModuleStarted.connect(
		    [&executionOver, &rootModuleId](uint32 moduleId) {
			    if(rootModuleId == moduleId)
			    {
				    executionOver = true;
			    }
		    });
		iopOs->StartModule(rootModuleId, "", nullptr, 0);
		iopOs->GetIoman()->SetFileStream(Iop::CIoman::FID_STDOUT, resultStream);
	}
	virtualMachine.Resume();

	while(!executionOver)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	virtualMachine.Pause();
	virtualMachine.Destroy();
}

void ScanAndExecuteTests(const boost::filesystem::path& testDirPath, const TestReportWriterPtr& testReportWriter, const std::string& gsHandlerName)
{
	boost::filesystem::directory_iterator endIterator;
	for(auto testPathIterator = boost::filesystem::directory_iterator(testDirPath);
	    testPathIterator != endIterator; testPathIterator++)
	{
		auto testPath = testPathIterator->path();
		if(boost::filesystem::is_directory(testPath))
		{
			ScanAndExecuteTests(testPath, testReportWriter, gsHandlerName);
			continue;
		}
		if(testPath.extension() == ".elf")
		{
			printf("Testing '%s': ", testPath.string().c_str());
			ExecuteEeTest(testPath, gsHandlerName);
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
		auto validGsHandlerNamesString =
		    []() {
			    std::string result;
			    for(auto nameIterator = g_validGsHandlersNames.begin();
			        nameIterator != g_validGsHandlersNames.end(); ++nameIterator)
			    {
				    if(nameIterator != g_validGsHandlersNames.begin())
				    {
					    result += "|";
				    }
				    result += *nameIterator;
			    }
			    return result;
		    }();

		printf("Usage: AutoTest [options] testDir\r\n");
		printf("Options: \r\n");
		printf("\t --junitreport <path>\t Writes JUnit format report at <path>.\r\n");
		printf("\t --gshandler <%s>\tSelects which GS handler to instantiate (default is '%s').\r\n",
		       validGsHandlerNamesString.c_str(), DEFAULT_GS_HANDLER_NAME);
		return -1;
	}

	TestReportWriterPtr testReportWriter;
	boost::filesystem::path autoTestRoot;
	boost::filesystem::path reportPath;
	std::string gsHandlerName = DEFAULT_GS_HANDLER_NAME;
	assert(g_validGsHandlersNames.find(gsHandlerName) != std::end(g_validGsHandlersNames));

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
		else if(!strcmp(argv[i], "--gshandler"))
		{
			if((i + 1) >= argc)
			{
				printf("Error: GS handler name must be specified for --gshandler option.\r\n");
				return -1;
			}
			gsHandlerName = argv[i + 1];
			if(g_validGsHandlersNames.find(gsHandlerName) == std::end(g_validGsHandlersNames))
			{
				printf("Error: Invalid GS handler name '%s'.\r\n", gsHandlerName.c_str());
				return -1;
			}
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

	try
	{
		ScanAndExecuteTests(autoTestRoot, testReportWriter, gsHandlerName);
	}
	catch(const std::exception& exception)
	{
		printf("Error: Failed to execute tests: %s\r\n", exception.what());
		return -1;
	}

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
