#include "PsfBase.h"
#include "MemStream.h"
#include <assert.h>
#include <string.h>
#include <stdexcept>
#include <zlib.h>

using namespace Framework;
using namespace std;

CPsfBase::CPsfBase(CStream& stream) :
m_reserved(NULL),
m_program(NULL)
{
    char signature[4];
    stream.Read(signature, 3);
    signature[3] = 0;
    if(strcmp(signature, "PSF"))
    {
        throw runtime_error("Invalid PSF file (Invalid signature).");
    }
    m_version = stream.Read8();
    m_reservedSize = stream.Read32();
    m_programSize = stream.Read32();
    m_programCrc = stream.Read32();

	if(m_reservedSize != 0)
	{
		m_reserved = new uint8[m_reservedSize];
		stream.Read(m_reserved, m_reservedSize);
	}

	if(m_programSize != 0)
	{
		ReadProgram(stream);
	}

	ReadTags(stream);
}

CPsfBase::~CPsfBase()
{
	delete [] m_reserved;
	delete [] m_program;
}

uint8* CPsfBase::GetProgram() const
{
	return m_program;
}

void CPsfBase::ReadProgram(CStream& stream)
{
	assert(m_program == NULL);

	CMemStream outputStream;
	uint8* compressedProgram = new uint8[m_programSize];
	stream.Read(compressedProgram, m_programSize);

	{
		z_stream zStream;
		const int bufferSize = 0x4000;
		uint8 buffer[bufferSize];
		memset(&zStream, 0, sizeof(zStream));
		inflateInit(&zStream);
		zStream.avail_in	= m_programSize;
		zStream.next_in		= reinterpret_cast<Bytef*>(compressedProgram);
		while(1)
		{
			zStream.avail_out	= bufferSize;
			zStream.next_out	= reinterpret_cast<Bytef*>(buffer);
			int result = inflate(&zStream, 0);
			if(result < 0)
			{
				throw runtime_error("Error occured while trying to decompress.");
			}
			outputStream.Write(buffer, bufferSize - zStream.avail_out);
			if(result == Z_STREAM_END)
			{
				break;
			}
		}
		inflateEnd(&zStream);
	}

	delete [] compressedProgram;

	{
		m_program = new uint8[outputStream.GetSize()];
		memcpy(m_program, outputStream.GetBuffer(), outputStream.GetSize());
	}
}

void CPsfBase::ReadTags(CStream& stream)
{
	char signature[6];
	stream.Read(signature, 5);
	signature[5] = 0;
	if(strcmp(signature, "[TAG]"))
	{
		return;
	}
	string line = "";
	while(1)
	{
		char nextCharacter = stream.Read8();
		if(stream.IsEOF()) break;
		if(nextCharacter == 0x0D)
		{
			continue;
		}
		else if(nextCharacter == 0x0A)
		{
			const char* tagBegin = line.c_str();
			const char* tagSeparator = strchr(line.c_str(), '=');
			const char* tagEnd = tagBegin + line.length();
			if(tagSeparator != NULL)
			{
				string tagName = string(tagBegin, tagSeparator);
				string tagValue = string(tagSeparator + 1, tagEnd);
				TagMap::iterator tagIterator(m_tags.find(tagName));
				if(tagIterator != m_tags.end())
				{
					m_tags[tagName] += string("/n") + tagValue;
				}
				else
				{
					m_tags[tagName] = tagValue;
				}
			}
			line = "";
		}
		else
		{
			line += nextCharacter;
		}
	}
}
