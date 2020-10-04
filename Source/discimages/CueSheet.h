#pragma once

#include <memory>
#include <vector>
#include "Stream.h"

class CCueSheet
{
public:
	struct COMMAND
	{
		virtual ~COMMAND() = default;
	};

	typedef std::unique_ptr<COMMAND> CommandPtr;
	typedef std::vector<CommandPtr> CommandList;

	struct COMMAND_FILE : public COMMAND
	{
		std::string filename;
		std::string filetype;
	};

	struct COMMAND_TRACK : public COMMAND
	{
		uint32 number;
		std::string datatype;
	};

	struct COMMAND_INDEX : public COMMAND
	{
		uint32 number;
		std::string time;
	};

	CCueSheet() = default;
	CCueSheet(Framework::CStream&);

	const CommandList& GetCommands() const;

private:
	void Read(Framework::CStream&);
	std::string ReadCommand(Framework::CStream&);
	std::string ReadPath(Framework::CStream&);

	std::vector<CommandPtr> m_commands;
};
