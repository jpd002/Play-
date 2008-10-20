#ifndef _PSFDEVICE_H_
#define _PSFDEVICE_H_

#include "iop/Ioman_Device.h"
#include "../PsfBase.h"
#include "PtrStream.h"
#include <memory>
#include <list>

namespace PS2
{
	class CPsfDevice : public Iop::Ioman::CDevice
	{
	public:
        typedef std::tr1::shared_ptr<CPsfBase> PsfFile;

					                    CPsfDevice(const PsfFile&);
		virtual		                    ~CPsfDevice();

        virtual Framework::CStream*     GetFile(uint32, const char*);

	private:
        struct NODE
        {
            virtual ~NODE()
            {

            }

            bool IsDirectory() const
            {
                return (size == 0 && blockSize == 0 && offset != 0);
            }

            char name[37];
            uint32 offset;
            uint32 size;
            uint32 blockSize;
        };

        struct FILE : public NODE
        {
            FILE(const NODE& baseNode) :
            NODE(baseNode)
            {
                data = new uint8[baseNode.size];
            }

            virtual ~FILE()
            {
                delete [] data;
            }

            uint8* data;
        };

        struct DIRECTORY : public NODE
        {
            typedef std::list<NODE*> FileListType;

            DIRECTORY()
            {

            }

            DIRECTORY(const NODE& baseNode) :
            NODE(baseNode)
            {

            }

            virtual ~DIRECTORY()
            {
                for(FileListType::const_iterator nodeIterator(fileList.begin());
                    fileList.end() != nodeIterator; nodeIterator++)
                {
                    delete (*nodeIterator);
                }
            }

            FileListType fileList;
        };

        static void                     ReadFile(Framework::CStream&, FILE&);
        static void                     ReadDirectory(Framework::CStream&, DIRECTORY&);

        const FILE*                     GetFileDetail(const DIRECTORY&, const char*) const;
        const NODE*                     GetFileFindNode(const DIRECTORY&, const char*) const;

        Framework::CPtrStream           m_fsStream;
        PsfFile                         m_psfFile;
        DIRECTORY                       m_root;
	};
}

#endif
