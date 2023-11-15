#include "make_unique.h"
#include "JUnitTestReportWriter.h"
#include "StdStreamUtils.h"
#include "xml/Writer.h"
#include "string_format.h"

CJUnitTestReportWriter::CJUnitTestReportWriter()
{
	m_reportNode = std::make_unique<Framework::Xml::CNode>("", true);
	auto testSuitesNode = m_reportNode->InsertNode(std::make_unique<Framework::Xml::CNode>("testsuites", true));
	m_testSuiteNode = testSuitesNode->InsertNode(std::make_unique<Framework::Xml::CNode>("testsuite", true));
}

CJUnitTestReportWriter::~CJUnitTestReportWriter()
{
}

void CJUnitTestReportWriter::ReportTestEntry(const std::string& testName, const TESTRESULT& result)
{
	auto testCaseNode = std::make_unique<Framework::Xml::CNode>("testcase", true);
	testCaseNode->InsertAttribute("name", testName.c_str());

	if(!result.succeeded)
	{
		std::string failureDetails;
		for(const auto& lineDiff : result.lineDiffs)
		{
			auto failureLine = string_format(
			    "result   : %s\r\n"
			    "expected : %s\r\n"
			    "\r\n",
			    lineDiff.result.c_str(), lineDiff.expected.c_str());
			failureDetails += failureLine;
		}
		auto resultNode = std::make_unique<Framework::Xml::CNode>("failure", true);
		resultNode->InsertTextNode(failureDetails.c_str());
		testCaseNode->InsertNode(std::move(resultNode));
	}

	m_testSuiteNode->InsertNode(std::move(testCaseNode));

	m_testCount++;
}

void CJUnitTestReportWriter::Write(const fs::path& reportPath)
{
	m_testSuiteNode->InsertAttribute("tests", string_format("%d", m_testCount).c_str());
	auto testOutputFileStream = Framework::CreateOutputStdStream(reportPath.native());
	Framework::Xml::CWriter::WriteDocument(testOutputFileStream, m_reportNode.get());
}
