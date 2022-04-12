#include "RegViewWnd.h"
#include "RegViewGeneral.h"
#include "RegViewSCU.h"
#include "RegViewFPU.h"
#include "RegViewVU.h"

CRegViewWnd::CRegViewWnd(QWidget* parent, CMIPS* ctx)
    : QTabWidget(parent)
{
	resize(320, 700);

	setTabPosition(QTabWidget::South);

	m_regView[0] = new CRegViewGeneral(this, ctx);
	m_regView[1] = new CRegViewSCU(this, ctx);
	m_regView[2] = new CRegViewFPU(this, ctx);
	m_regView[3] = new CRegViewVU(this, ctx);

	addTab(m_regView[0], "General");
	addTab(m_regView[1], "SCU");
	addTab(m_regView[2], "FPU");
	addTab(m_regView[3], "VU");
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
