#include "CallStackWnd.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"
#include "DebugUtils.h"
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"

#include <QVBoxLayout>
#include <QLabel>

CCallStackWnd::CCallStackWnd(QWidget* parent, CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
    : QListWidget(parent)
    , m_context(context)
    , m_biosDebugInfoProvider(biosDebugInfoProvider)
{
	resize(320, 750);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	connect(this, &QListWidget::itemDoubleClicked, this, &CCallStackWnd::listDoubleClick);

	Update();
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

	clear();

	auto callStackItems = CMIPSAnalysis::GetCallStack(m_context, nPC, nSP, nRA);

	if(callStackItems.size() == 0)
	{
		addItem("Call stack unavailable at this state");
		return;
	}

	auto modules = m_biosDebugInfoProvider ? m_biosDebugInfoProvider->GetModulesDebugInfo() : BiosDebugModuleInfoArray();

	for(const auto& callStackItem : callStackItems)
	{
		auto locationString = DebugUtils::PrintAddressLocation(callStackItem, m_context, modules);
		addItem(locationString.c_str());
	}
}

void CCallStackWnd::listDoubleClick(QListWidgetItem* item)
{
	std::string addressStr = item->text().toStdString();
	uint32 nAddress = lexical_cast_hex(addressStr);
	if(nAddress != MIPS_INVALID_PC)
	{
		OnFunctionDblClick(nAddress);
	}
}
