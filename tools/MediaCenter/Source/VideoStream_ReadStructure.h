#ifndef _VIDEOSTREAM_READSTRUCTURE_H_
#define _VIDEOSTREAM_READSTRUCTURE_H_

#include <assert.h>
#include <vector>
#include <boost/any.hpp>
#include "VideoStream_Program.h"

namespace VideoStream
{
	template <typename StructType>
	class ReadStructure : public Program
	{
	public:
		enum COMMAND_TYPE
		{
			COMMAND_TYPE_READ8,
			COMMAND_TYPE_READ16,
			COMMAND_TYPE_READ32,
			COMMAND_TYPE_CHECKMARKER,
			COMMAND_ALIGN8,
		};

		struct COMMAND
		{
			COMMAND(COMMAND_TYPE type, uint8 size, const boost::any& payload)
			    : type(type)
			    , size(size)
			    , payload(payload)
			{
			}

			COMMAND_TYPE type;
			uint8 size;
			boost::any payload;
		};

		ReadStructure()
		{
		}

		virtual ~ReadStructure()
		{
		}

		void InsertCommand(const COMMAND& command)
		{
			m_commands.push_back(command);
		}

		void Reset()
		{
			m_currentCommandIndex = 0;
		}

		void Execute(void* context, Framework::CBitStream& stream)
		{
			StructType& structure(*reinterpret_cast<StructType*>(context));
			while(1)
			{
				if(m_currentCommandIndex == m_commands.size()) return;
				const COMMAND& command(m_commands[m_currentCommandIndex]);
				switch(command.type)
				{
				case COMMAND_TYPE_READ8:
				{
					assert(command.size <= 8);
					StructOffset8 offset = boost::any_cast<StructOffset8>(command.payload);
					structure.*(offset) = static_cast<uint8>(stream.GetBits_MSBF(command.size));
				}
				break;
				case COMMAND_TYPE_READ16:
				{
					assert(command.size <= 16);
					StructOffset16 offset = boost::any_cast<StructOffset16>(command.payload);
					structure.*(offset) = static_cast<uint16>(stream.GetBits_MSBF(command.size));
				}
				break;
				case COMMAND_TYPE_READ32:
				{
					assert(command.size <= 32);
					StructOffset32 offset = boost::any_cast<StructOffset32>(command.payload);
					structure.*(offset) = stream.GetBits_MSBF(command.size);
				}
				break;
				case COMMAND_TYPE_CHECKMARKER:
				{
					uint32 marker = stream.GetBits_MSBF(command.size);
					uint32 reference = boost::any_cast<int>(command.payload);
					assert(marker == reference);
				}
				break;
				case COMMAND_ALIGN8:
				{
					stream.SeekToByteAlign();
				}
				break;
				default:
					assert(0);
					break;
				}
				m_currentCommandIndex++;
			}
		}

	protected:
		typedef uint8 StructType::*StructOffset8;
		typedef uint16 StructType::*StructOffset16;
		typedef uint32 StructType::*StructOffset32;
		typedef std::vector<COMMAND> CommandList;

		CommandList m_commands;
		unsigned int m_currentCommandIndex;
	};
}

#endif
