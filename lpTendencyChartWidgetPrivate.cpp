#include "lpTendencyChartWidgetPrivate.h"

lpTendencyChartWidgetPrivate::lpTendencyChartWidgetPrivate(lpTendencyChartWidget *parent)
	: QObject(parent)
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
	if (m_dataChart)
	{
		delete m_dataChart;
	}
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

	m_dataChart = new lpTendencyDataChart(this, ui.Chartwidget, curveNames, m_ChartConfig);
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


	connect(m_ChartConfig, &lpTendencyChartConfig::curveDisplayChanged, m_dataChart, &lpTendencyDataChart::onCurveDisplayChanged);
	connect(ui.Interval_PB, &QPushButton::clicked, this, &lpTendencyChartWidgetPrivate::handleIntervalPBClicked);
	connect(ui.Toggle_PB, &QPushButton::clicked, this, &lpTendencyChartWidgetPrivate::toggleTableVisibility);
	connect(ui.Align_PB, &QPushButton::clicked, this, &lpTendencyChartWidgetPrivate::AlignPBClicked);
	connect(m_ChartConfig, &lpTendencyChartConfig::curveDisplayChanged, m_dataScope, &lpTendencyDataScope::setColumnVisibility);
	connect(this, &lpTendencyChartWidgetPrivate::sgClearScope, m_dataScope, &lpTendencyDataScope::clearTable);
	connect(this, &lpTendencyChartWidgetPrivate::sgClearChart, m_dataChart, &lpTendencyDataChart::clearChart);
	//connect(m_ChartConfig, &lpTendencyChartConfig::curveNamesChanged, m_dataScope, &lpTendencyDataScope::setColumnNames);


//处理接收到的数据处理线程
	m_manageThread = new lpTendencyManageThread();
	m_thread = new QThread(this);
	m_manageThread->moveToThread(m_thread);

	connect(this, &lpTendencyChartWidgetPrivate::sgUpdateDataLithium, m_manageThread, &lpTendencyManageThread::onUpdateDataScope);
	connect(m_manageThread, &lpTendencyManageThread::sgupdateScope, this, &lpTendencyChartWidgetPrivate::updateDataScope);
	connect(m_manageThread, &lpTendencyManageThread::sgupdateChart, this, &lpTendencyChartWidgetPrivate::updateDataChart);
	m_thread->start();



//通用
	/*connect(this, &lpTendencyChartWidgetPrivate::sgUpdateData, this, &lpTendencyChartWidgetPrivate::updateDataScope);
	connect(this, &lpTendencyChartWidgetPrivate::sgUpdateData, this, &lpTendencyChartWidgetPrivate::updateDataChart);
*/


}




void lpTendencyChartWidgetPrivate::handleIntervalPBClicked() {
	if (m_dataChart) {
		m_dataChart->onIntervalPBClicked();//用于参数设置对话框
	}
}


void lpTendencyChartWidgetPrivate::toggleTableVisibility()
{
	ui.treeWidget->setVisible(!ui.treeWidget->isVisible());
	ui.Toggle_PB->setText(ui.treeWidget->isVisible() ? "趋势指标勾选隐藏" : "趋势指标勾选显示");
}

void lpTendencyChartWidgetPrivate::AlignPBClicked()
{
	if (m_dataChart)
	{
		m_dataChart->AlignPBClicked();//用于显示对齐度设置对话框
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
	m_dataChart->onChartUpdate(curveName, x, y);

}




