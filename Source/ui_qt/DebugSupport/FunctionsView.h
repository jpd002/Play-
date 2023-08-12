#pragma once

#include "TagsView.h"

class CFunctionsView : public CTagsView
{
	Q_OBJECT

public:
	CFunctionsView(QMdiArea*);
	virtual ~CFunctionsView() = default;

	void SetContext(CMIPS*, CBiosDebugInfoProvider*);

public slots:
	void OnImportClick();
};
