#include "QBootablesView.h"
#include "ui_bootableview.h"

#include <QAction>

#include "CoverUtils.h"
#include "ui_shared/BootablesProcesses.h"

QBootablesView::QBootablesView(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QBootablesView)

{
	ui->setupUi(this);
	ui->listView->setStyleSheet("QListView{ background:QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #4baaf3, stop: 1 #0A0A0A); }");

	// used as workaround to avoid direct ui access from a thread
	connect(this, SIGNAL(AsyncUpdateCoverDisplay()), this, SLOT(UpdateCoverDisplay()));
	connect(this, SIGNAL(AsyncResetModel(bool)), this, SLOT(resetModel(bool)));

	ui->listView->setContextMenuPolicy(Qt::ActionsContextMenu);

	ui->listView->setItemDelegate(new BootImageItemDelegate);

	m_bootables = BootablesDb::CClient::GetInstance().GetBootables(BootablesDb::CClient::SORT_METHOD::SORT_METHOD_RECENT);

	CoverUtils::PopulatePlaceholderCover();
	AsyncPopulateCache();

	auto model = new BootableModel(this, m_bootables);
	ui->listView->setModel(model);

	connect(ui->listView, &QAbstractItemView::doubleClicked, this, &QBootablesView::DoubleClicked);
}

QBootablesView::~QBootablesView()
{
	if(m_coverLoader.joinable())
		m_coverLoader.join();
}

void QBootablesView::AddAction(std::string name, Callback callback)
{
	auto action = new QAction(name.c_str(), this);
	auto listViewBoundCallback = [listView = ui->listView, callback](bool val) {
		callback(listView, val);
	};
	connect(action, &QAction::triggered, listViewBoundCallback);
	ui->listView->addAction(action);
}

void QBootablesView::AddBootAction(Callback callback)
{
	auto action = new QAction("Boot", this);
	auto listViewBoundCallback = [listView = ui->listView, callback](bool val) {
		callback(listView, val);
	};
	connect(action, &QAction::triggered, listViewBoundCallback);
	ui->listView->addAction(action);
	m_bootCallback = callback;
}

void QBootablesView::DoubleClicked(const QModelIndex&)
{
	// we assume, selection will change on click, thus we can ignore the index
	m_bootCallback(ui->listView, true);
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
	static_cast<BootableModel*>(ui->listView->model())->SetWidth(size().width() - style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 5);
	QWidget::resizeEvent(ev);
}