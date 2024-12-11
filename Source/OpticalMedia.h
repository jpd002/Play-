#pragma once

#include <vector>
#include "Stream.h"
#include "ISO9660/ISO9660.h"

namespace ISO9660
{
	class CBlockProvider;
}

class COpticalMedia
{
public:
	constexpr static uint64 MEDIA_BLOCK_SIZE_2352 = 2352;
	
	enum MEDIA_BLOCK_TYPE
	{
		MEDIA_BLOCK_TYPE_2352, //CD
		MEDIA_BLOCK_TYPE_2048, //DVD, UMD, BD
	};

	enum CREATE_FLAGS
	{
		CREATE_AUTO_DISABLE_DL_DETECT = 0x01,
		CREATE_AUTO_NO_FIRST_TRACK = 0x02,
	};

	struct TRACK
	{
		uint32 start = 0;
		uint32 pregap = 0; //Track Index 1
		uint32 size = 0;
	};

	typedef std::shared_ptr<Framework::CStream> StreamPtr;
	typedef std::shared_ptr<ISO9660::CBlockProvider> BlockProviderPtr;

	COpticalMedia() = default;

	static std::unique_ptr<COpticalMedia> CreateAuto(const StreamPtr&, uint32 = 0);
	static std::unique_ptr<COpticalMedia> CreateDvd(const StreamPtr&, bool = false, uint32 = 0);
	static std::unique_ptr<COpticalMedia> CreateCustom(BlockProviderPtr, MEDIA_BLOCK_TYPE, std::vector<TRACK>);

	ISO9660::CBlockProvider* GetBlockProvider() const;
	MEDIA_BLOCK_TYPE GetMediaBlockType() const;

	void AddTrack(const TRACK&);
	uint32 GetTrackCount() const;
	const TRACK& GetTrack(uint32) const;

	CISO9660* GetFileSystem();
	CISO9660* GetFileSystemL1();

	bool GetDvdIsDualLayer() const;
	uint32 GetDvdSecondLayerStart() const;

private:
	typedef std::unique_ptr<CISO9660> Iso9660Ptr;

	void CheckDualLayerDvd(const StreamPtr&);
	void SetupSecondLayer(const StreamPtr&);

	MEDIA_BLOCK_TYPE m_mediaBlockType = MEDIA_BLOCK_TYPE_2048;
	BlockProviderPtr m_blockProvider;
	std::vector<TRACK> m_tracks;
	bool m_dvdIsDualLayer = false;
	uint32 m_dvdSecondLayerStart = 0;
	Iso9660Ptr m_fileSystem;
	Iso9660Ptr m_fileSystemL1;
};
