#pragma once

#include <QTimer>
#include "FutureContinuationManager.h"

class CContinuationChecker : public QObject
{
	Q_OBJECT

public:
	CContinuationChecker(QObject*);
	CFutureContinuationManager& GetContinuationManager();

public slots:
	void updateContinuations();

private:
	QTimer* m_timer = nullptr;
	CFutureContinuationManager m_continuationManager;
};
