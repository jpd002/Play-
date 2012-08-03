#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

#include "PsfTags.h"
#include <vector>
#include <boost/signals2.hpp>

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

		std::wstring				path;
		std::wstring				title;
		double						length;
		unsigned int				id;
		unsigned int				archiveId;
	};

	typedef boost::signals2::signal<void (const ITEM&)>					OnItemInsertEvent;
	typedef boost::signals2::signal<void (unsigned int, const ITEM&)>	OnItemUpdateEvent;
	typedef boost::signals2::signal<void (unsigned int)>				OnItemDeleteEvent;
	typedef boost::signals2::signal<void ()>							OnItemsClearEvent;

									CPlaylist();
	virtual							~CPlaylist();

	const ITEM&						GetItem(unsigned int) const;
	int								FindItem(unsigned int) const;
	unsigned int					GetItemCount() const;

	void							Read(const boost::filesystem::path&);
	void							Write(const boost::filesystem::path&);

	static bool						IsLoadableExtension(const char*);
	static void						PopulateItemFromTags(ITEM&, const CPsfTags&);

	unsigned int					InsertArchive(const wchar_t*);
	std::wstring					GetArchive(unsigned int) const;

	unsigned int					InsertItem(const ITEM&);
	void							UpdateItem(unsigned int, const ITEM&);
	void							DeleteItem(unsigned int);
	void							ExchangeItems(unsigned int, unsigned int);
	void							Clear();

	OnItemInsertEvent				OnItemInsert;
	OnItemUpdateEvent				OnItemUpdate;
	OnItemDeleteEvent				OnItemDelete;
	OnItemsClearEvent				OnItemsClear;

private:
	typedef std::vector<ITEM> ItemList;
	typedef std::vector<std::wstring> ArchiveList;
	typedef ItemList::const_iterator ItemIterator;

	static const char*				g_loadableExtensions[];
	ItemList						m_items;
	ArchiveList						m_archives;
	unsigned int					m_currentItemId;
};

#endif
