#include <QHeaderView>
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "ThreadsViewWnd.h"
#include "DebugUtils.h"
#include "QtDialogListWidget.h"

CThreadsViewWnd::CThreadsViewWnd(QWidget* parent)
    : QTableView(parent)
    , m_context(nullptr)
    , m_biosDebugInfoProvider(nullptr)
{
	resize(300, 700);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	std::vector<std::string> headers = {"Id", "Priority", "Location", "State"};
	m_model = new CQtGenericTableModel(this, {"Id", "Priority", "Location", "State"});
	setModel(m_model);
	auto header = horizontalHeader();
	header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(1, QHeaderView::Interactive);
	header->setSectionResizeMode(2, QHeaderView::Stretch);
	header->setSectionResizeMode(3, QHeaderView::Stretch);
	verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	verticalHeader()->hide();
	setSelectionBehavior(QAbstractItemView::SelectRows);

	QString text("Priority");
	QFontMetrics fm = fontMetrics();
	int width = fm.width(text);
	header->resizeSection(1, width);

	connect(this, &QTableView::doubleClicked, this, &CThreadsViewWnd::tableDoubleClick);
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

void CThreadsViewWnd::tableDoubleClick(const QModelIndex& indexRow)
{
	auto index = m_model->index(indexRow.row(), 0);
	auto threadId = std::stoi(m_model->getItem(index));

	auto threadInfos = m_biosDebugInfoProvider->GetThreadsDebugInfo();

	auto threadInfoIterator = std::find_if(std::begin(threadInfos), std::end(threadInfos),
	                                       [&](const BIOS_DEBUG_THREAD_INFO& threadInfo) { return threadInfo.id == threadId; });
	if(threadInfoIterator == std::end(threadInfos)) return;

	const auto& threadInfo(*threadInfoIterator);

	auto callStackItems = CMIPSAnalysis::GetCallStack(m_context, threadInfo.pc, threadInfo.sp, threadInfo.ra);
	if(callStackItems.size() <= 1)
	{
		OnGotoAddress(threadInfo.pc);
	}
	else
	{
		std::map<std::string, uint32> addrMap;
		QtDialogListWidget dialog(this);
		for(auto itemIterator(std::begin(callStackItems));
		    itemIterator != std::end(callStackItems); itemIterator++)
		{
			const auto& item(*itemIterator);
			std::string locationString = DebugUtils::PrintAddressLocation(item, m_context, m_biosDebugInfoProvider->GetModulesDebugInfo());
			dialog.addItem(locationString);
			addrMap.insert({locationString, item});
		}

		dialog.exec();
		auto locationString = dialog.getResult();
		if(!locationString.empty())
		{
			auto address = addrMap.at(locationString);
			OnGotoAddress(address);
		}
	}
}
