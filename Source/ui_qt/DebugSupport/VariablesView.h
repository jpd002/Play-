#pragma once

#include "TagsView.h"

class CVariablesView : public CTagsView
{
	Q_OBJECT

public:
	CVariablesView(QMdiArea*);
	virtual ~CVariablesView() = default;

	void SetContext(CMIPS*, CBiosDebugInfoProvider*);
};
