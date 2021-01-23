#pragma once

#include <map>
#include <memory>
#include "Iop_Module.h"
#include "Ioman_Defs.h"
#include "Ioman_Device.h"
#include "Stream.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CIopBios;

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

		class FileNotFoundException : public std::exception
		{
		};

		typedef std::shared_ptr<Ioman::CDevice> DevicePtr;

		CIoman(CIopBios&, uint8*);
		virtual ~CIoman();

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void SaveState(Framework::CZipArchiveWriter&) const override;
		void LoadState(Framework::CZipArchiveReader&) override;

		void RegisterDevice(const char*, const DevicePtr&);

		uint32 Open(uint32, const char*);
		uint32 Close(uint32);
		uint32 Read(uint32, uint32, void*);
		uint32 Write(uint32, uint32, const void*);
		uint32 Seek(uint32, int32, uint32);
		int32 Mkdir(const char* path);
		int32 Dopen(const char*);
		int32 Dread(uint32, Ioman::DIRENTRY*);
		int32 Dclose(uint32);
		uint32 GetStat(const char*, Ioman::STAT*);
		uint32 DelDrv(uint32);
		int32 Mount(const char*, const char*);
		int32 Umount(const char*);
		uint64 Seek64(uint32, int64, uint32);

		//These are to be called from VM code, because they might
		//execute user device code
		int32 OpenVirtual(CMIPS&);
		int32 CloseVirtual(CMIPS&);
		int32 ReadVirtual(CMIPS&);
		int32 WriteVirtual(CMIPS&);
		int32 SeekVirtual(CMIPS&);
		int32 AddDrv(CMIPS&);

		uint32 GetFileMode(uint32) const;

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
				std::swap(descPtr, rhs.descPtr);
			}

			void Reset()
			{
				delete stream;
				stream = nullptr;
				flags = 0;
				descPtr = 0;
				path.clear();
			}

			FileInfo& operator=(const FileInfo&) = delete;
			FileInfo(const FileInfo&) = delete;

			Framework::CStream* stream = nullptr;
			uint32 descPtr = 0;
			std::string path;
			uint32 flags = 0;
		};

		typedef std::map<int32, FileInfo> FileMapType;
		typedef std::map<uint32, Ioman::Directory> DirectoryMapType;
		typedef std::map<std::string, DevicePtr> DeviceMapType;
		typedef std::map<std::string, uint32> UserDeviceMapType;

		void PrepareOpenThunk();
		Framework::CStream* OpenInternal(uint32, const char*);
		int32 AllocateFileHandle();
		void FreeFileHandle(uint32);
		int32 PreOpen(uint32, const char*);

		static Framework::STREAM_SEEK_DIRECTION ConvertWhence(uint32);

		void InvokeUserDeviceMethod(CMIPS&, uint32, size_t offset, uint32 arg0 = 0, uint32 arg1 = 0, uint32 arg2 = 0);

		bool IsUserDeviceFileHandle(int32) const;
		uint32 GetUserDeviceFileDescPtr(int32) const;

		void SaveFilesState(Framework::CZipArchiveWriter&) const;
		void SaveUserDevicesState(Framework::CZipArchiveWriter&) const;

		void LoadFilesState(Framework::CZipArchiveReader&);
		void LoadUserDevicesState(Framework::CZipArchiveReader&);

		FileMapType m_files;
		DirectoryMapType m_directories;
		DeviceMapType m_devices;
		UserDeviceMapType m_userDevices;
		CIopBios& m_bios;
		uint8* m_ram;
		uint32 m_nextFileHandle;
		uint32 m_openThunkPtr = 0;
	};

	typedef std::shared_ptr<CIoman> IomanPtr;
}
