#pragma once

#include <memory>
#include "BlockProvider.h"
#include "VolumeDescriptor.h"
#include "PathTable.h"
#include "DirectoryRecord.h"

class CISO9660
{
public:
	typedef std::shared_ptr<ISO9660::CBlockProvider> BlockProviderPtr;

								CISO9660(const BlockProviderPtr&);
								~CISO9660();

	void						ReadBlock(uint32, void*);

	Framework::CStream*			Open(const char*);
	bool						GetFileRecord(ISO9660::CDirectoryRecord*, const char*);

private:
	bool						GetFileRecordFromDirectory(ISO9660::CDirectoryRecord*, uint32, const char*);

	BlockProviderPtr			m_blockProvider;
	ISO9660::CVolumeDescriptor	m_volumeDescriptor;
	ISO9660::CPathTable			m_pathTable;

	uint8						m_blockBuffer[ISO9660::CBlockProvider::BLOCKSIZE];
};
