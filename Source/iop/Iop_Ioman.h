#pragma once

#include <map>
#include <memory>
#include "Iop_Module.h"
#include "Ioman_Device.h"
#include "Stream.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
	class CIoman : public CModule
	{
	public:
		enum
		{
			FID_STDOUT = 1,
			FID_STDERR = 2
		};

		enum
		{
			SEEK_DIR_SET = 0,
			SEEK_DIR_CUR = 1,
			SEEK_DIR_END = 2
		};

		struct STAT
		{
			uint32 mode;
			uint32 attr;
			uint32 loSize;
			uint8 creationTime[8];
			uint8 lastAccessTime[8];
			uint8 lastModificationTime[8];
			uint32 hiSize;
			//Reserved (private) fields to be used in later versions of IOMAN
			//uint32 reserved[6];
		};
		static_assert(sizeof(STAT) == 40, "STAT structure must be 40 bytes long.");

		struct DIRENTRY
		{
			enum
			{
				NAME_SIZE = 256,
			};

			STAT stat;
			char name[NAME_SIZE];
			uint32 privatePtr;
		};
		static_assert(sizeof(DIRENTRY) == 0x12C, "DIRENTRY structure must be 300 bytes long");

		class CFile
		{
		public:
			CFile(uint32, CIoman&);
			CFile(const CFile&) = delete;
			virtual ~CFile();

			operator uint32();
			CFile& operator=(const CFile&) = delete;

		private:
			uint32 m_handle;
			CIoman& m_ioman;
		};

		typedef std::shared_ptr<Ioman::CDevice> DevicePtr;

		CIoman(uint8*);
		virtual ~CIoman();

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void SaveState(Framework::CZipArchiveWriter&);
		void LoadState(Framework::CZipArchiveReader&);

		void RegisterDevice(const char*, const DevicePtr&);

		uint32 Open(uint32, const char*);
		uint32 Close(uint32);
		uint32 Read(uint32, uint32, void*);
		uint32 Write(uint32, uint32, const void*);
		uint32 Seek(uint32, uint32, uint32);
		int32 Dopen(const char*);
		int32 Dread(uint32, DIRENTRY*);
		int32 Dclose(uint32);
		uint32 GetStat(const char*, STAT*);
		uint32 AddDrv(uint32);
		uint32 DelDrv(uint32);

		Framework::CStream* GetFileStream(uint32);
		void SetFileStream(uint32, Framework::CStream*);

	private:
		struct FileInfo
		{
			FileInfo() = default;

			FileInfo(Framework::CStream* stream)
			    : stream(stream)
			{
			}

			FileInfo(FileInfo&& rhs)
			{
				MoveFrom(std::move(rhs));
			}

			~FileInfo()
			{
				Reset();
			}

			FileInfo& operator=(FileInfo&& rhs)
			{
				MoveFrom(std::move(rhs));
				return (*this);
			}

			void MoveFrom(FileInfo&& rhs)
			{
				Reset();
				std::swap(stream, rhs.stream);
				std::swap(path, rhs.path);
				std::swap(flags, rhs.flags);
			}

			void Reset()
			{
				delete stream;
				stream = nullptr;
				flags = 0;
				path.clear();
			}

			FileInfo& operator=(const FileInfo&) = delete;
			FileInfo(const FileInfo&) = delete;

			Framework::CStream* stream = nullptr;
			std::string path;
			uint32 flags = 0;
		};

		typedef std::map<uint32, FileInfo> FileMapType;
		typedef std::map<uint32, Ioman::Directory> DirectoryMapType;
		typedef std::map<std::string, DevicePtr> DeviceMapType;

		Framework::CStream* OpenInternal(uint32, const char*);

		FileMapType m_files;
		DirectoryMapType m_directories;
		DeviceMapType m_devices;
		uint8* m_ram;
		uint32 m_nextFileHandle;
	};

	typedef std::shared_ptr<CIoman> IomanPtr;
}
