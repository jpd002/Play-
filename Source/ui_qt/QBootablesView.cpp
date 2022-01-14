#include "QBootablesView.h"

#include <QAction>

#include "CoverUtils.h"
#include "ui_shared/BootablesProcesses.h"

QBootablesView::QBootablesView(QWidget* parent)
{

	setStyleSheet("QBootablesView{ background:QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #4baaf3, stop: 1 #0A0A0A); }");

	// used as workaround to avoid direct ui access from a thread
	connect(this, SIGNAL(AsyncUpdateCoverDisplay()), this, SLOT(UpdateCoverDisplay()));
	connect(this, SIGNAL(AsyncResetModel(bool)), this, SLOT(resetModel(bool)));

	QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(sizePolicy.hasHeightForWidth());
	setSizePolicy(sizePolicy);
	setMovement(QListView::Free);
	setResizeMode(QListView::Adjust);
	setViewMode(QListView::IconMode);
	setUniformItemSizes(true);

	setContextMenuPolicy(Qt::ActionsContextMenu);

	setItemDelegate(new BootImageItemDelegate);

	m_bootables = BootablesDb::CClient::GetInstance().GetBootables(BootablesDb::CClient::SORT_METHOD::SORT_METHOD_RECENT);

	CoverUtils::PopulatePlaceholderCover();
	AsyncPopulateCache();

	auto model = new BootableModel(this, m_bootables);
	setModel(model);

	connect(this, &QAbstractItemView::doubleClicked, this, &QBootablesView::DoubleClicked);
}

QBootablesView::~QBootablesView()
{
	if(m_coverLoader.joinable())
		m_coverLoader.join();
}

void QBootablesView::AddAction(std::string name, Callback callback)
{
	auto action = new QAction(name.c_str(), this);
	connect(action, &QAction::triggered, callback);
	addAction(action);
}

void QBootablesView::AddBootAction(Callback callback)
{
	auto action = new QAction("Boot", this);
	connect(action, &QAction::triggered, callback);
	addAction(action);
	m_bootCallback = callback;
}

void QBootablesView::DoubleClicked(const QModelIndex&)
{
	// we assume, selection will change on click, thus we can ignore the index
	m_bootCallback(true);
}

void QBootablesView::AsyncPopulateCache()
{
	if(!m_threadRunning)
	{
		if(m_coverLoader.joinable())
			m_coverLoader.join();

		m_threadRunning = true;
		m_coverLoader = std::thread([&] {
			CoverUtils::PopulateCache(m_bootables);

			AsyncUpdateCoverDisplay();
			m_threadRunning = false;
		});
	}
}

void QBootablesView::resizeEvent(QResizeEvent* ev)
{
	static_cast<BootableModel*>(model())->SetWidth(size().width() - style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 5);
	QListView::resizeEvent(ev);
}