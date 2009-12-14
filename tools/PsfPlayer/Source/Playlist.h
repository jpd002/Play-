#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

#include "PsfTags.h"
#include <boost/signal.hpp>
#include <vector>

class CPlaylist
{
public:
    struct ITEM
    {
        std::string                 path;
        std::wstring                title;
        double                      length;
    };

    typedef boost::signal<void (const ITEM&)>                   OnItemInsertEvent;
    typedef boost::signal<void (unsigned int, const ITEM&)>     OnItemUpdateEvent;
    typedef boost::signal<void (unsigned int)>                  OnItemDeleteEvent;
    typedef boost::signal<void ()>                              OnItemsClearEvent;

                                    CPlaylist();
    virtual                         ~CPlaylist();

    const ITEM&                     GetItem(unsigned int) const;
    unsigned int                    GetItemCount() const;

    void                            Read(const char*);
    void                            Write(const char*);

    static void                     PopulateItemFromTags(ITEM&, const CPsfTags&);

    void                            InsertItem(const ITEM&);
    void                            UpdateItem(unsigned int, const ITEM&);
    void                            DeleteItem(unsigned int);
	void							ExchangeItems(unsigned int, unsigned int);
    void                            Clear();

    OnItemInsertEvent               OnItemInsert;
    OnItemUpdateEvent               OnItemUpdate;
    OnItemDeleteEvent               OnItemDelete;
    OnItemsClearEvent               OnItemsClear;

private:
    typedef std::vector<ITEM> ItemList;
    typedef ItemList::const_iterator ItemIterator;

    ItemList                        m_items;
};

#endif
