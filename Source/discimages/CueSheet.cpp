#include <cassert>
#include <stdexcept>
#include "CueSheet.h"
#include "string_format.h"

//Based on 'Appendix A of the CDRWIN User's Guide'
//http://web.archive.org/web/20070221154246/http://www.goldenhawk.com/download/cdrwin.pdf

CCueSheet::CCueSheet(Framework::CStream& stream)
{
	Read(stream);
}

const CCueSheet::CommandList& CCueSheet::GetCommands() const
{
	return m_commands;
}

void CCueSheet::Read(Framework::CStream& stream)
{
	while(1)
	{
		auto nextCommand = ReadCommand(stream);
		if(stream.IsEOF()) break;
		if(nextCommand == "FILE")
		{
			auto fileCommand = std::make_unique<COMMAND_FILE>();
			fileCommand->filename = ReadPath(stream);
			fileCommand->filetype = ReadCommand(stream);
			m_commands.push_back(std::move(fileCommand));
		}
		else if(nextCommand == "TRACK")
		{
			auto trackCommand = std::make_unique<COMMAND_TRACK>();
			trackCommand->number = atoi(ReadCommand(stream).c_str());
			trackCommand->datatype = ReadCommand(stream);
			m_commands.push_back(std::move(trackCommand));
		}
		else if(nextCommand == "INDEX")
		{
			auto indexCommand = std::make_unique<COMMAND_INDEX>();
			indexCommand->number = atoi(ReadCommand(stream).c_str());
			indexCommand->time = ReadCommand(stream);
			m_commands.push_back(std::move(indexCommand));
		}
		else if(nextCommand == "REM")
		{
			//Ignore remarks
			auto remark = stream.ReadLine();
		}
		else
		{
			auto message = string_format("Unknown command '%s' encountered.", nextCommand.c_str());
			throw std::runtime_error(message);
		}
	}
}

std::string CCueSheet::ReadCommand(Framework::CStream& stream)
{
	enum class STATE
	{
		TRIM_SPACE,
		READING,
		DONE,
	};
	auto state = STATE::TRIM_SPACE;
	std::string result;
	while(state != STATE::DONE)
	{
		char nextChar = stream.Read8();
		if(stream.IsEOF()) break;
		switch(state)
		{
		case STATE::TRIM_SPACE:
			if(!isspace(nextChar))
			{
				result += nextChar;
				state = STATE::READING;
			}
			break;
		case STATE::READING:
			if(isspace(nextChar))
			{
				state = STATE::DONE;
			}
			else
			{
				result += nextChar;
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	return result;
}

std::string CCueSheet::ReadPath(Framework::CStream& stream)
{
	enum class STATE
	{
		TRIM_SPACE,
		READING_NORMAL,
		READING_QUOTED,
		DONE,
	};
	auto state = STATE::TRIM_SPACE;
	std::string result;
	while(state != STATE::DONE)
	{
		char nextChar = stream.Read8();
		if(stream.IsEOF()) break;
		switch(state)
		{
		case STATE::TRIM_SPACE:
			if(nextChar == '\"')
			{
				state = STATE::READING_QUOTED;
			}
			else if(!isspace(nextChar))
			{
				result += nextChar;
				state = STATE::READING_NORMAL;
			}
			break;
		case STATE::READING_QUOTED:
			if(nextChar == '\"')
			{
				state = STATE::DONE;
			}
			else
			{
				result += nextChar;
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	return result;
}
