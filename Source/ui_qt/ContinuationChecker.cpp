#include "ContinuationChecker.h"

CContinuationChecker::CContinuationChecker(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
	connect(m_timer, SIGNAL(timeout()), this, SLOT(updateContinuations()));
	m_timer->start(250);
}

CFutureContinuationManager& CContinuationChecker::GetContinuationManager()
{
	return m_continuationManager;
}

void CContinuationChecker::updateContinuations()
{
	m_continuationManager.Execute();
}
