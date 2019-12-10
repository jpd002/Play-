#include <QHeaderView>
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "ThreadsViewWnd.h"
// #include "ThreadCallStackViewWnd.h"
#include "DebugUtils.h"

CThreadsViewWnd::CThreadsViewWnd(QMdiArea* parent)
    : QMdiSubWindow(parent)
    , m_context(nullptr)
    , m_biosDebugInfoProvider(nullptr)
{

	setMinimumHeight(700);
	setMinimumWidth(300);

	parent->addSubWindow(this)->setWindowTitle("Threads");

	m_tableView = new QTableView(this);
	setWidget(m_tableView);
	m_tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	std::vector<std::string> headers = {"Id", "Priority", "Location", "State"};
	m_model = new CQtGenericTableModel(parent, {"Id", "Priority", "Location", "State"});
	m_tableView->setModel(m_model);
	m_tableView->horizontalHeader()->setStretchLastSection(true);
	m_tableView->resizeColumnsToContents();
}

void CThreadsViewWnd::HandleMachineStateChange()
{
	Update();
}

void CThreadsViewWnd::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	m_context = context;
	m_biosDebugInfoProvider = biosDebugInfoProvider;

	Update();
}

void CThreadsViewWnd::Update()
{
	m_model->clear();

	if(!m_biosDebugInfoProvider) return;

	auto threadInfos = m_biosDebugInfoProvider->GetThreadsDebugInfo();
	auto moduleInfos = m_biosDebugInfoProvider->GetModulesDebugInfo();

	for(auto threadInfoIterator(std::begin(threadInfos));
	    threadInfoIterator != std::end(threadInfos); threadInfoIterator++)
	{
		const auto& threadInfo = *threadInfoIterator;

		std::vector<std::string> data;
		data.push_back(lexical_cast_uint<std::string>(threadInfo.id));
		data.push_back(lexical_cast_uint<std::string>(threadInfo.priority));

		std::string locationString = DebugUtils::PrintAddressLocation(threadInfo.pc, m_context, moduleInfos);
		data.push_back(locationString);

		data.push_back(threadInfo.stateDescription);
		m_model->addItem(data);
	}
}

// void CThreadsViewWnd::OnListDblClick()
// {
// 	int nSelection = m_listView.GetSelection();
// 	if(nSelection == -1) return;

// 	uint32 threadId = m_listView.GetItemData(nSelection);

// 	auto threadInfos = m_biosDebugInfoProvider->GetThreadsDebugInfo();

// 	auto threadInfoIterator = std::find_if(std::begin(threadInfos), std::end(threadInfos),
// 	                                       [&](const BIOS_DEBUG_THREAD_INFO& threadInfo) { return threadInfo.id == threadId; });
// 	if(threadInfoIterator == std::end(threadInfos)) return;

// 	const auto& threadInfo(*threadInfoIterator);

// 	auto callStackItems = CMIPSAnalysis::GetCallStack(m_context, threadInfo.pc, threadInfo.sp, threadInfo.ra);
// 	if(callStackItems.size() <= 1)
// 	{
// 		OnGotoAddress(threadInfo.pc);
// 	}
// 	else
// 	{
// 		CThreadCallStackViewWnd threadCallStackViewWnd(m_hWnd);
// 		threadCallStackViewWnd.SetItems(m_context, callStackItems, m_biosDebugInfoProvider->GetModulesDebugInfo());
// 		threadCallStackViewWnd.DoModal();

// 		if(threadCallStackViewWnd.HasSelection())
// 		{
// 			OnGotoAddress(threadCallStackViewWnd.GetSelectedAddress());
// 		}
// 	}
// }
