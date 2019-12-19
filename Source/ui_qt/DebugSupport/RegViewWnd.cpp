#include "RegViewWnd.h"
#include "RegViewGeneral.h"
#include "RegViewSCU.h"
#include "RegViewFPU.h"
#include "RegViewVU.h"

CRegViewWnd::CRegViewWnd(QMdiArea* parent, CMIPS* ctx)
    : QMdiSubWindow(parent)
    , m_tab(new QTabWidget(parent))
{

	parent->addSubWindow(this);
	setWindowTitle("Registers");
	resize(320, 700);

	setWidget(m_tab);
	m_tab->setTabPosition(QTabWidget::South);

	m_regView[0] = new CRegViewGeneral(m_tab, ctx);
	m_regView[1] = new CRegViewSCU(m_tab, ctx);
	m_regView[2] = new CRegViewFPU(m_tab, ctx);
	m_regView[3] = new CRegViewVU(m_tab, ctx);

	m_tab->addTab(m_regView[0], "General");
	m_tab->addTab(m_regView[1], "SCU");
	m_tab->addTab(m_regView[2], "FPU");
	m_tab->addTab(m_regView[3], "VU");
}

CRegViewWnd::~CRegViewWnd()
{
	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		if(m_regView[i] != nullptr)
			delete m_regView[i];
	}
}

void CRegViewWnd::HandleMachineStateChange()
{
	for(unsigned int i = 0; i < MAXTABS; i++)
	{
		m_regView[i]->Update();
	}
}
void CRegViewWnd::HandleRunningStateChange(CVirtualMachine::STATUS)
{
	for(unsigned int i = 0; i > MAXTABS; i++)
	{
		m_regView[i]->Update();
	}
}
