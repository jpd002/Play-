#ifndef _ELFSYMBOLVIEW_H_
#define _ELFSYMBOLVIEW_H_

#include "win32/Window.h"
#include "win32/ListView.h"
#include "../ELF.h"
#include <boost/flyweight.hpp>

class CELFSymbolView : public Framework::Win32::CWindow
{
public:
									CELFSymbolView(HWND, CELF*);
	virtual							~CELFSymbolView();

private:
	typedef boost::flyweight<std::tstring> PooledString;

	struct ITEM
	{
		PooledString	name;
		PooledString	address;
		PooledString	size;
		PooledString	type;
		PooledString	binding;
		PooledString	section;
	};

	typedef std::vector<ITEM> ItemArray;

	enum SORT_STATE
	{
		SORT_STATE_NONE,
		SORT_STATE_NAME_ASC,
		SORT_STATE_NAME_DESC,
		SORT_STATE_ADDRESS_ASC,
		SORT_STATE_ADDRESS_DESC,
	};

	long							OnSize(unsigned int, unsigned int, unsigned int);
	long							OnNotify(WPARAM, NMHDR*);

	static int						ItemNameComparer(const ITEM&, const ITEM&);
	static int						ItemAddressComparer(const ITEM&, const ITEM&);

	void							RefreshLayout();
	void							PopulateList();
	void							GetItemInfo(LVITEM*) const;

	CELF*							m_pELF;
	Framework::Win32::CListView*	m_listView;
	ItemArray						m_items;

	SORT_STATE						m_sortState;
};

#endif