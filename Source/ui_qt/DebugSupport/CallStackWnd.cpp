#include "CallStackWnd.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"
#include "DebugUtils.h"
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"

#define CLSNAME _T("CallStackWnd")
#include <QVBoxLayout>
#include <QLabel>

CCallStackWnd::CCallStackWnd(QMdiArea* parent, CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
    : QMdiSubWindow(parent)
	, m_context(context)
    , m_biosDebugInfoProvider(biosDebugInfoProvider)
{
	setMinimumHeight(750);
	setMinimumWidth(320);

	parent->addSubWindow(this)->setWindowTitle("Call Stack");

	m_listWidget = new QListWidget(this);
	setWidget(m_listWidget);
	m_listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &CCallStackWnd::listDoubleClick);

	Update();
}

CCallStackWnd::~CCallStackWnd()
{
}

void CCallStackWnd::HandleMachineStateChange()
{
	Update();
}

void CCallStackWnd::Update()
{
	uint32 nPC = m_context->m_State.nPC;
	uint32 nRA = m_context->m_State.nGPR[CMIPS::RA].nV[0];
	uint32 nSP = m_context->m_State.nGPR[CMIPS::SP].nV[0];

	m_listWidget->clear();

	auto callStackItems = CMIPSAnalysis::GetCallStack(m_context, nPC, nSP, nRA);

	if(callStackItems.size() == 0)
	{
		m_listWidget->addItem("Call stack unavailable at this state");
		return;
	}

	auto modules = m_biosDebugInfoProvider ? m_biosDebugInfoProvider->GetModulesDebugInfo() : BiosDebugModuleInfoArray();

	for(const auto& callStackItem : callStackItems)
	{
		auto locationString = DebugUtils::PrintAddressLocation(callStackItem, m_context, modules);
		m_listWidget->addItem(locationString.c_str());
	}
}

void CCallStackWnd::listDoubleClick(QListWidgetItem *item)
{
	std::string addressStr = item->text().toStdString();
	uint32 nAddress = lexical_cast_hex(addressStr);
	if(nAddress != MIPS_INVALID_PC)
	{
		OnFunctionDblClick(nAddress);
	}
}
