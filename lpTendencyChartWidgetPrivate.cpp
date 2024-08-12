#include "lpTendencyChartWidgetPrivate.h"
#pragma execution_character_set("utf-8")
lpTendencyChartWidgetPrivate::lpTendencyChartWidgetPrivate(lpTendencyChartWidget *parent)
	: QObject(parent), m_numCharts(4) // 默认为4个图表
{


	ui.setupUi(parent);
	init();
	qDebug() << "This is main Thread...";
	qDebug() << "Test !!!";
	qDebug() << "Test3";
	//connect(ui.btn_save, &QPushButton::clicked, this, &lpTendencyChartWidgetPrivate::save);
}

lpTendencyChartWidgetPrivate::~lpTendencyChartWidgetPrivate()
{
	if (m_dataScope)
	{
		//QStringList ChooseNames = m_ChartConfig->getCurveNames();
		//m_dataScope->saveTableSettings(ChooseNames);
		//m_dataScope->saveSettingsToFile();
		m_dataScope->saveColumnConfig();
		delete m_dataScope;

	}
	//if (m_dataChart)
	//{
	//	delete m_dataChart;
	//}
	if (m_ChartConfig)
	{
		m_ChartConfig->saveConfig("Config/Event.json");
		delete m_ChartConfig;
	}

	if (m_thread && m_thread->isRunning()) {
		m_manageThread->stopThread();
		m_thread->quit();
		m_thread->wait();
	}
	delete m_manageThread;

}


void lpTendencyChartWidgetPrivate::init()
{
	m_ChartConfig = new lpTendencyChartConfig(ui.treeWidget, this);
	m_ChartConfig->loadConfig("Config/Event.json");

	QStringList curveNames = m_ChartConfig->getCurveNames(); // 获取曲线名称
	QStringList allCurveNames = m_ChartConfig->getAllCurveNames(); // 获取所有曲线名称
	//QStringList ChooseNames = m_ChartConfig->getCurveNames();

    m_gridLayout = new QGridLayout();

    m_scrollArea = new QScrollArea(ui.Chartwidget);
    m_scrollAreaWidgetContents = new QWidget();
    m_scrollAreaWidgetContents->setLayout(m_gridLayout);

    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setWidget(m_scrollAreaWidgetContents);

    QVBoxLayout *chartWidgetLayout = new QVBoxLayout(ui.Chartwidget);
    chartWidgetLayout->addWidget(m_scrollArea);
    ui.Chartwidget->setLayout(chartWidgetLayout);

    setupCharts();

    //int numCharts = 4; // 可以根据需要调整图表数量
    //for (int i = 0; i < numCharts; ++i) {
    //    // 为每个图表创建一个新的 lpTendencyDataChart 实例
    //    m_dataChart = new lpTendencyDataChart(this, ui.Chartwidget, curveNames, m_ChartConfig);
    //    // 将图表添加到网格布局中，这里简单地分布在网格的行中
    //    gridLayout->addWidget(m_dataChart->getPlot(), i / 2, i % 2); // 2列布局
    //}

	//m_dataChart = new lpTendencyDataChart(this, ui.Chartwidget, curveNames, m_ChartConfig);
	if (!m_dataScope)
	{
		m_dataScope = new lpTendencyDataScope(ui.tableWidget, this);
	}
	m_dataScope->setColumnNames(allCurveNames);
	m_dataScope->loadColumnConfig();



	// 获取初始勾选状态并设置列的可见性
	QMap<QString, bool> initialDisplayStatus = m_ChartConfig->getInitialCurveDisplayStatus();
	for (const QString &curveName : allCurveNames) {
		bool isVisible = initialDisplayStatus.value(curveName, false); // 默认不显示
		m_dataScope->setColumnVisibility(curveName, isVisible);
	}


	//connect(m_ChartConfig, &lpTendencyChartConfig::curveDisplayChanged, m_dataChart, &lpTendencyDataChart::onCurveDisplayChanged);
	connect(ui.Interval_PB, &QPushButton::clicked, this, &lpTendencyChartWidgetPrivate::handleIntervalPBClicked);
	connect(ui.Toggle_PB, &QPushButton::clicked, this, &lpTendencyChartWidgetPrivate::toggleTableVisibility);
	connect(ui.Align_PB, &QPushButton::clicked, this, &lpTendencyChartWidgetPrivate::AlignPBClicked);
	connect(m_ChartConfig, &lpTendencyChartConfig::curveDisplayChanged, m_dataScope, &lpTendencyDataScope::setColumnVisibility);
	connect(this, &lpTendencyChartWidgetPrivate::sgClearScope, m_dataScope, &lpTendencyDataScope::clearTable);
	//connect(this, &lpTendencyChartWidgetPrivate::sgClearChart, m_dataChart, &lpTendencyDataChart::clearChart);
	
	//connect(m_ChartConfig, &lpTendencyChartConfig::curveNamesChanged, m_dataScope, &lpTendencyDataScope::setColumnNames);


//处理接收到的数据处理线程
	m_manageThread = new lpTendencyManageThread();
	m_thread = new QThread(this);
	m_manageThread->moveToThread(m_thread);

	connect(this, &lpTendencyChartWidgetPrivate::sgUpdateDataLithium, m_manageThread, &lpTendencyManageThread::onUpdateDataScope);
	connect(m_manageThread, &lpTendencyManageThread::sgupdateScope, this, &lpTendencyChartWidgetPrivate::updateDataScope);
	connect(m_manageThread, &lpTendencyManageThread::sgupdateChart, this, &lpTendencyChartWidgetPrivate::updateDataChart);
	m_thread->start();
    connect(ui.ChartNum_PB, &QPushButton::clicked, this, &lpTendencyChartWidgetPrivate::onChangeChartNumClicked);


//通用
	/*connect(this, &lpTendencyChartWidgetPrivate::sgUpdateData, this, &lpTendencyChartWidgetPrivate::updateDataScope);
	connect(this, &lpTendencyChartWidgetPrivate::sgUpdateData, this, &lpTendencyChartWidgetPrivate::updateDataChart);
*/


}




void lpTendencyChartWidgetPrivate::handleIntervalPBClicked() {

    if (!m_dataCharts.isEmpty())
    {
        m_dataCharts.first()->onIntervalPBClicked();
    }

}


void lpTendencyChartWidgetPrivate::toggleTableVisibility()
{
	ui.treeWidget->setVisible(!ui.treeWidget->isVisible());
	ui.Toggle_PB->setText(ui.treeWidget->isVisible() ? "趋势指标勾选隐藏" : "趋势指标勾选显示");
}

void lpTendencyChartWidgetPrivate::AlignPBClicked()
{
    if (!m_dataCharts.isEmpty())
    {
        m_dataCharts.first()->AlignPBClicked();
    }
}

QWidget * lpTendencyChartWidgetPrivate::getTopWidget()
{
	return ui.Top_widget;
}

QWidget * lpTendencyChartWidgetPrivate::getDownWidget()
{
	return ui.Down_widget;
}


void lpTendencyChartWidgetPrivate::updateDataScope(const QString &curveName, double x, double y)
{
	qDebug() << "updateDataScopeThreadID: " << QThread::currentThreadId();
	// 获取当前曲线所属的父类名称
	QString parentName = m_ChartConfig->getParentNameForCurve(curveName);
	// 获取该父类的设置默认值
	QVariantMap settingDefaults = m_ChartConfig->getSettingDefaultValue(parentName);

	QVariantList warningValue, alarmValue;
	if (settingDefaults.contains("warningValue")) {
		warningValue = settingDefaults["warningValue"].toList();
	}
	if (settingDefaults.contains("alarmValue")) {
		alarmValue = settingDefaults["alarmValue"].toList();
	}

	// 使用已有的addData函数来添加数据到表格，并传递告警值和预警值范围
	m_dataScope->addData(curveName, x, y, warningValue, alarmValue);

}

/*趋势图*/

void lpTendencyChartWidgetPrivate::updateDataChart(const QString &curveName, double x, double y) {

	qDebug() << "updateDataChartThreadID: " << QThread::currentThreadId();
    for (lpTendencyDataChart *chart : m_dataCharts) {
        chart->onChartUpdate(curveName, x, y); // 更新每个图表
    }

}


void lpTendencyChartWidgetPrivate::onChangeChartNumClicked()
{
    bool ok;
    int num = QInputDialog::getInt(nullptr, "趋势图数量", "请输入趋势图个数:", m_numCharts, 1, 8, 1, &ok);
    if (ok) {
        setChartCount(num);
    }
}



void lpTendencyChartWidgetPrivate::setChartCount(int count)
{
    if (count != m_numCharts) {
        m_numCharts = count;

        // 清除旧的图表和布局
        // 先移除所有控件，防止它们被自动删除
        QLayoutItem* item;
        while ((item = m_gridLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->setParent(nullptr);
            }
            delete item;
        }

        delete m_gridLayout; // 现在安全删除布局

        // 创建新的布局和图表
        m_gridLayout = new QGridLayout(m_scrollAreaWidgetContents);
        setupCharts();
    }
}

void lpTendencyChartWidgetPrivate::setupCharts()
{
    QStringList curveNames = m_ChartConfig->getCurveNames();
    int rows = std::sqrt(m_numCharts);
    int cols = (m_numCharts + rows - 1) / rows;
 // 清除旧的图表列表
    m_dataCharts.clear();

    for (int i = 0; i < m_numCharts; ++i) {
        QWidget *chartContainer = new QWidget(m_scrollAreaWidgetContents);
        QVBoxLayout *chartLayout = new QVBoxLayout(chartContainer);

        lpTendencyDataChart *chart = new lpTendencyDataChart(this, chartContainer, curveNames, m_ChartConfig);
        m_dataCharts.append(chart);

        // 将图表的绘图部分添加到布局中
        chartLayout->addWidget(chart->getPlot());
        chartLayout->addWidget(chart->getSlider());  // 确保滑动条在图表下方

        // 设置容器的布局
        chartContainer->setLayout(chartLayout);

        // 将容器添加到网格布局中
        m_gridLayout->addWidget(chartContainer, i / cols, i % cols);

        connect(m_ChartConfig, &lpTendencyChartConfig::curveDisplayChanged, chart, &lpTendencyDataChart::onCurveDisplayChanged);
    }
}