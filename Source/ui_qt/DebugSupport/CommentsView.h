#pragma once

#include "TagsView.h"

class CCommentsView : public CTagsView
{
	Q_OBJECT

public:
	CCommentsView(QMdiArea*);
	virtual ~CCommentsView() = default;

	void SetContext(CMIPS*, CBiosDebugInfoProvider*);
};
