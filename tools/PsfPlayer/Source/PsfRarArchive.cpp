#include "PsfRarArchive.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include <unrar/rar.hpp>
#include "stricmp.h"

static Archive* ConvertArchive(void* archivePtr)
{
	return reinterpret_cast<Archive*>(archivePtr);
}

CPsfRarArchive::CPsfRarArchive()
{
	m_archive = new Archive();
}

CPsfRarArchive::~CPsfRarArchive()
{
	delete ConvertArchive(m_archive);
}

void CPsfRarArchive::Open(const char* path)
{
	Archive* arc(ConvertArchive(m_archive));
	arc->Open(path);
	arc->IsArchive(false);
	if(!arc->IsOpened())
	{
		throw std::runtime_error("Couldn't open archive.");
	}

	while(arc->ReadHeader() > 0)
	{
		if(arc->ShortBlock.HeadType == FILE_HEAD)
		{
			if(!arc->IsArcDir())
			{
				FILEINFO fileInfo;
				fileInfo.name = arc->NewLhd.FileName;
				boost::replace_all(fileInfo.name, "\\", "/");
				fileInfo.length = static_cast<unsigned int>(arc->NewLhd.FullUnpSize);
				m_files.push_back(fileInfo);
			}
		}
		arc->SeekToNext();
	}
}

void CPsfRarArchive::ReadFileContents(const char* fileName, void* buffer, unsigned int bufferLength)
{
	Archive* arc(ConvertArchive(m_archive));
	if(!arc->IsOpened())
	{
		throw std::runtime_error("Archive isn't opened.");
	}

	std::string fixedFileName(fileName);
	boost::replace_all(fixedFileName, "/", "\\");

	arc->Seek(0, SEEK_SET);

	ComprDataIO dataIo;
	dataIo.Init();

	Unpack unpack(&dataIo);
	unpack.Init(NULL);

	while(arc->ReadHeader() > 0)
	{
		if(arc->ShortBlock.HeadType == FILE_HEAD)
		{
			if(!arc->IsArcDir())
			{
				bool isGoodFile = !stricmp(fixedFileName.c_str(), arc->NewLhd.FileName);

				dataIo.SetFiles(arc, NULL);
				dataIo.SetPackedSizeToRead(arc->NewLhd.FullPackSize);

				dataIo.CurUnpRead = 0;
				dataIo.CurUnpWrite = 0;
				dataIo.UnpFileCRC = arc->OldFormat ? 0 : 0xFFFFFFFF;
				dataIo.PackedCRC = 0xFFFFFFFF;

				dataIo.SetTestMode(!isGoodFile);

				if(isGoodFile)
				{
					dataIo.SetUnpackToMemory(reinterpret_cast<byte*>(buffer), bufferLength);
				}

				unpack.SetDestSize(arc->NewLhd.FullUnpSize);

				if(arc->NewLhd.Method == 0x30)
				{
					std::vector<byte> unstoreBuffer;
					unstoreBuffer.resize(0x10000);
					uint toReadSize = arc->NewLhd.FullUnpSize;
					while(1)
					{
						uint code = dataIo.UnpRead(&unstoreBuffer[0], unstoreBuffer.size());
						if(code == 0 || code == -1) break;
						code = code < toReadSize ? code : toReadSize;
						dataIo.UnpWrite(&unstoreBuffer[0], code);
						if(toReadSize >= 0)
						{
							toReadSize -= code;
						}
					}
				}
				else
				{
					unpack.DoUnpack(arc->NewLhd.UnpVer, (arc->NewLhd.Flags & LHD_SOLID) != 0);
				}

				if(arc->NewLhd.FileCRC != ~dataIo.UnpFileCRC)
				{
					throw std::runtime_error("CRC check error.");
				}

				if(isGoodFile)
				{
					return;
				}
			}
		}
		arc->SeekToNext();
	}

	throw std::runtime_error("Couldn't read file from archive.");
}
