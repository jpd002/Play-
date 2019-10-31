#pragma once

#include "PsfTags.h"
#include <vector>
#include "signal/Signal.h"
#include "filesystem_def.h"

class CPlaylist
{
public:
	struct ITEM
	{
		ITEM()
		    : length(0)
		    , id(0)
		    , archiveId(0)
		{
		}

		std::wstring path;
		std::wstring title;
		double length;
		unsigned int id;
		unsigned int archiveId;
	};

	typedef Framework::CSignal<void(const ITEM&)> OnItemInsertEvent;
	typedef Framework::CSignal<void(unsigned int, const ITEM&)> OnItemUpdateEvent;
	typedef Framework::CSignal<void(unsigned int)> OnItemDeleteEvent;
	typedef Framework::CSignal<void()> OnItemsClearEvent;

	CPlaylist();
	virtual ~CPlaylist();

	const ITEM& GetItem(unsigned int) const;
	int FindItem(unsigned int) const;
	unsigned int GetItemCount() const;

	void Read(const fs::path&);
	void Write(const fs::path&);

	static bool IsLoadableExtension(const std::string&);
	static void PopulateItemFromTags(ITEM&, const CPsfTags&);

	unsigned int InsertArchive(const std::wstring&);
	std::wstring GetArchive(unsigned int) const;

	unsigned int InsertItem(const ITEM&);
	void UpdateItem(unsigned int, const ITEM&);
	void DeleteItem(unsigned int);
	void ExchangeItems(unsigned int, unsigned int);
	void Clear();

	OnItemInsertEvent OnItemInsert;
	OnItemUpdateEvent OnItemUpdate;
	OnItemDeleteEvent OnItemDelete;
	OnItemsClearEvent OnItemsClear;

private:
	typedef std::vector<ITEM> ItemList;
	typedef std::vector<std::wstring> ArchiveList;

	static const char* g_loadableExtensions[];
	ItemList m_items;
	ArchiveList m_archives;
	unsigned int m_currentItemId;
};
