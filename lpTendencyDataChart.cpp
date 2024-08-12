#include "lpTendencyDataChart.h"
#pragma execution_character_set("utf-8")

lpTendencyDataChart::lpTendencyDataChart(QObject *parent, QWidget *parentWidget, const QStringList &curveNames,
	lpTendencyChartConfig* configLoader)
	: QObject(parent), m_widget(parentWidget), m_curveNames(curveNames),
	m_configLoader(configLoader)
{
	m_plot = new QwtPlot(m_widget);
	//plot->setTitle("实时趋势图");
	m_plot->setCanvasBackground(Qt::white);

	// 启用图例
	m_plot->insertLegend(new QwtLegend(), QwtPlot::TopLegend);

	// 设置X轴和Y轴的标题
	m_plot->setAxisTitle(QwtPlot::xBottom, "");
	m_plot->setAxisTitle(QwtPlot::yLeft, "");
	// 设置自定义的 ScaleDraw
	m_plot->setAxisScaleDraw(QwtPlot::xBottom, new CustomScaleDraw());

	m_slider = new QSlider(Qt::Horizontal, m_widget);

	QString selectedParentNames = m_configLoader->getSelectedParentNames();
	if (!selectedParentNames.isEmpty())
	{
		QVariantMap settingDefaults = m_configLoader->getSettingDefaultValue(selectedParentNames);
		if (settingDefaults.contains("yAxisRange"))
		{
			QVariantList yAxisRange = settingDefaults["yAxisRange"].toList();
			if (yAxisRange.size() == 2) {
				double minY = yAxisRange[0].toDouble();
				double maxY = yAxisRange[1].toDouble();
				m_plot->setAxisScale(QwtPlot::yLeft, minY, maxY);
			}
		}
		m_plot->replot();
	}
	// 设置X轴和Y轴的初始范围
	m_plot->setAxisScale(QwtPlot::xBottom, 0, 50); // 设置X轴的范围为0-50
	//plot->setAxisScale(QwtPlot::yLeft, 0, 900); // 设置Y轴的范围为0-900
	m_plot->setCanvasBackground(QColor(14, 22, 55));


	// 创建并配置网格
	QwtPlotGrid *grid = new QwtPlotGrid();
	grid->attach(m_plot); // 将网格附加到图表
	grid->setVisible(false); // 隐藏网格线


		// 确保curveNames包含所有曲线名称
	QStringList allCurveNames = m_configLoader->getAllCurveNames();
	for (const QString &name : allCurveNames) {
		addCurve(name);  // 添加所有曲线到图表
	}


	if (m_widget)
	{
		// 使用QSplitter代替原来的布局
		QSplitter *splitter = new QSplitter(Qt::Vertical, m_widget);
		splitter->addWidget(m_plot);
		splitter->addWidget(m_slider);
		QVBoxLayout *layout = new QVBoxLayout(m_widget);
		layout->addWidget(splitter);
		m_widget->setLayout(layout);

	}

	QwtLegend *legend = new QwtLegend();
	legend->setDefaultItemMode(QwtLegendData::Clickable);
	m_plot->insertLegend(legend, QwtPlot::TopLegend);

	/*	m_slider->setRange(0, 1000); */ // 假设x的最大值是1000m
	connect(m_slider, &QSlider::valueChanged, this, &lpTendencyDataChart::onSliderValueChanged);
	connect(legend, SIGNAL(clicked(const QVariant &, int)), this, SLOT(onLegendClicked(const QVariant &, int)));
	connect(m_configLoader, &lpTendencyChartConfig::yAxisRangeChanged, this, &lpTendencyDataChart::updateYAxisRange);
	connect(m_configLoader, &lpTendencyChartConfig::warningValueChanged, this, &lpTendencyDataChart::updateWarningValue);
	connect(m_configLoader, &lpTendencyChartConfig::alarmValueChanged, this, &lpTendencyDataChart::updateAlarmValue);
	installEventFilters();

	m_updateTimer = new QTimer(this);
	connect(m_updateTimer, &QTimer::timeout, this, &lpTendencyDataChart::batchUpdateChart);
	m_updateTimer->start(1000); // 每1000毫秒（1秒）触发一次


}

lpTendencyDataChart::~lpTendencyDataChart() {

	m_updateTimer->stop();

}



double lpTendencyDataChart::adjustXValue(double originalX) {
	// 四舍五入到最近的0.1
	double adjustedX = std::round(originalX * 10) / 10.0;
	qDebug() << "adjustedX: " << adjustedX;
	return adjustedX;
}


void lpTendencyDataChart::onChartUpdate(const QString &curveName, double x, double y) {
	//if (x >= 30000) {
	//	// 清除所有曲线的数据
	//	for (auto &curve : m_curves) {
	//		m_xDataMap[curve->title().text()].clear();
	//		m_yDataMap[curve->title().text()].clear();
	//		curve->setSamples(m_xDataMap[curve->title().text()], m_yDataMap[curve->title().text()]);
	//	}
	//	// 重置x轴范围
	//	m_plot->setAxisScale(QwtPlot::xBottom, 0, 50);
	//	m_plot->replot();
	//	return; // 早期返回，避免在清除后添加数据点
	//}

	if (x > m_maxReceivedX) {
		m_maxReceivedX = x;
	}


	// 调整x值
	x = adjustXValue(x);


	// 检查x值是否超过了上次重置点+1000
	if (x - m_lastResetX >= METER_THRESHOLD) {
		m_lastResetX = x; // 更新上次重置点
		for (auto &curve : m_curves) {
			m_xDataMap[curve->title().text()].clear();
			m_yDataMap[curve->title().text()].clear();
			curve->setSamples(m_xDataMap[curve->title().text()], m_yDataMap[curve->title().text()]);
		}
		m_plot->setAxisScale(QwtPlot::xBottom, m_lastResetX, m_lastResetX + 50); // 重置X轴的范围为当前x到x+50
		m_plot->replot();
		updateSliderPosition(); // 更新滑块的位置和范围
		return; // 早期返回，避免在清除后添加数据点
	}

	// 原有的更新曲线数据的逻辑
	if (!m_xDataMap.contains(curveName) || !m_yDataMap.contains(curveName)) {
		return; // 如果曲线名称不存在，则直接返回
	}

	m_xDataMap[curveName] << x;
	m_yDataMap[curveName] << y;

	// 原有的逻辑，用于根据新的数据点更新x轴范围
	if (!m_isViewingHistory) {
		xInterval = 50; // 这里设置x轴的间隔值，根据实际情况调整
		int xMaxCurrent = m_plot->axisScaleDiv(QwtPlot::xBottom).upperBound(); // 获取当前x轴的最大值
		if (x >= xMaxCurrent) {
			int xMinNew = ((x / xInterval) * xInterval);
			int xMaxNew = xMinNew + xInterval;
			m_plot->setAxisScale(QwtPlot::xBottom, xMinNew, xMaxNew);
		}
	}

	// 更新数据点后，调整滑动条的最大值
	int currentMaxX = static_cast<int>(m_maxReceivedX);
	m_slider->setMaximum(currentMaxX);

	// 保持滑块在最右端
	if (m_slider->value() == m_slider->maximum() - 50) { // 假设显示范围是50
		m_slider->setValue(m_slider->maximum());
	}
	m_hasNewData = true; // 设置有新数据的标志
	m_plot->replot(); // 重绘图表以应用更改
}


void lpTendencyDataChart::batchUpdateChart() {

	if (!m_hasNewData) {
		return; // 如果没有新数据，则不进行重绘
	}

	for (auto &curve : m_curves) {
		QString curveName = curve->title().text();
		if (m_xDataMap.contains(curveName) && m_yDataMap.contains(curveName)) {
			curve->setSamples(m_xDataMap[curveName], m_yDataMap[curveName]);
		}
	}

	m_plot->replot(); // 重绘图表
	m_replotCount++;
	qDebug() << "图表重绘次数：" << m_replotCount;

	m_hasNewData = false;
}



//参数设置
void lpTendencyDataChart::onIntervalPBClicked() {

	QDialog dialog(m_widget); // 使用当前widget作为父窗口
	dialog.setWindowTitle("参数设置");
	dialog.setStyleSheet("QDialog {background-color: black;}");
	//dialog.resize(650, 900);
	//dialog.setFixedSize(dialog.size());

	QScrollArea *scrollArea = new QScrollArea(&dialog);
	scrollArea->setWidgetResizable(true);
	QWidget *container = new QWidget();
	container->setStyleSheet("QWidget {background-color: black;}");



	QGridLayout *gridLayout = new QGridLayout(container);
	gridLayout->setMargin(0);


	// 通过ConfigLoader获取父类名字
	QStringList settingLabels = m_configLoader->getParentCategoryNames();
	QVector<QLineEdit*> rangeInputs1, rangeInputs2;
	QVector<QLineEdit*> warningInputs1, warningInputs2;
	QVector<QLineEdit*> alarmInputs1, alarmInputs2;

	//QStringList settingLabels = {
	//	"A/B面整体宽度",
	//	"A/B面电浆宽度",
	//	"A/B面左侧陶瓷宽度",
	//	"A/B面右侧陶瓷宽度",
	//	"A/B面对齐度"
	//};

	// 对于每个设置项，我们需要创建不同的控件
	for (int i = 0; i < settingLabels.size(); ++i) {

		int row = i * 5; // 每个设置占用5行
		QString settingName = settingLabels[i];
		// 从配置加载器获取默认值
		QVariantMap settingDefaults = m_configLoader->getSettingDefaultValue(settingName);
		// 设置标签
		QLabel* settingLabel = new QLabel(QString::number(i + 1) + "." + settingLabels[i], &dialog);
		settingLabel->setStyleSheet("QLabel { color: white; font-weight: bold; font-size: 14pt;}");
		gridLayout->addWidget(settingLabel, row, 0, 1, 2);

		// 趋势图y轴显示区域设置
		QHBoxLayout* rangeLayout = new QHBoxLayout;
		QLabel* rangeLabel = new QLabel("  设置趋势图y轴显示区域（毫米）：", &dialog);
		rangeLabel->setStyleSheet("QLabel {color: white; font-size: 12pt;}");
		QLineEdit* rangeInput1 = new QLineEdit(&dialog);
		rangeInput1->setStyleSheet("QLineEdit {color: white;}");
		rangeInput1->setValidator(new QDoubleValidator(-1000, 10000, 2, rangeInput1));
		//rangeInput1->setReadOnly(true); // 设置为只读
		QLabel *separator = new QLabel(" --- ", &dialog);
		separator->setStyleSheet("QLabel {color: white; }");
		QLineEdit* rangeInput2 = new QLineEdit(&dialog);
		rangeInput2->setStyleSheet("QLineEdit {color: white;}");
		rangeInput2->setValidator(new QDoubleValidator(-1000, 10000, 2, rangeInput2));
		//rangeInput2->setReadOnly(true); // 设置为只读
		QVariantList yAxisRange = settingDefaults["yAxisRange"].toList();
		if (!yAxisRange.isEmpty()) {
			rangeInput1->setText(QString::number(yAxisRange[0].toDouble(), 'f', 2));
			rangeInput2->setText(QString::number(yAxisRange[1].toDouble(), 'f', 2));
		}
		rangeInputs1.push_back(rangeInput1);
		rangeInputs2.push_back(rangeInput2);
		rangeLayout->addWidget(rangeLabel);
		rangeLayout->addWidget(rangeInput1);
		rangeLayout->addWidget(separator);
		rangeLayout->addWidget(rangeInput2);
		gridLayout->addLayout(rangeLayout, row + 1, 0, 1, 2);

		// 预警值设置
		QHBoxLayout* warningLayout = new QHBoxLayout;
		QLabel* warningLabel = new QLabel(QString("  设置%1预警值（毫米）：").arg(settingLabels[i]), &dialog);
		warningLabel->setStyleSheet("QLabel { color: orange; font-size: 12pt;}");
		QLabel* greaterWarningLabel = new QLabel("大于", &dialog);
		greaterWarningLabel->setStyleSheet("QLabel { color: white;}");
		QLineEdit* greaterWarningInput = new QLineEdit(&dialog);
		greaterWarningInput->setStyleSheet("QLineEdit {color: white;}");
		greaterWarningInput->setValidator(new QDoubleValidator(-1000, 10000, 2, greaterWarningInput));
		//greaterWarningInput->setReadOnly(true); // 设置为只读
		QLabel* lessWarningLabel = new QLabel("或小于", &dialog);
		lessWarningLabel->setStyleSheet("QLabel {color: white; }");
		QLineEdit* lessWarningInput = new QLineEdit(&dialog);
		lessWarningInput->setStyleSheet("QLineEdit {color: white;}");
		lessWarningInput->setValidator(new QDoubleValidator(-1000, 10000, 2, lessWarningInput));
		//lessWarningInput->setReadOnly(true); // 设置为只读
		QVariantList warningValue = settingDefaults["warningValue"].toList();
		if (!warningValue.isEmpty()) {
			greaterWarningInput->setText(QString::number(warningValue[0].toDouble(), 'f', 2));
			lessWarningInput->setText(QString::number(warningValue[1].toDouble(), 'f', 2));
		}
		warningInputs1.push_back(greaterWarningInput);
		warningInputs2.push_back(lessWarningInput);

		warningLayout->addWidget(warningLabel);
		warningLayout->addWidget(greaterWarningLabel);
		warningLayout->addWidget(greaterWarningInput);
		warningLayout->addWidget(lessWarningLabel);
		warningLayout->addWidget(lessWarningInput);
		gridLayout->addLayout(warningLayout, row + 2, 0, 1, 2);

		// 告警值设置
		QHBoxLayout* alarmLayout = new QHBoxLayout;
		QLabel* alarmLabel = new QLabel(QString("  设置%1告警值（毫米）：").arg(settingLabels[i]), &dialog);
		alarmLabel->setStyleSheet("QLabel { color: red; font-size: 12pt;}");
		QLabel* greaterAlarmLabel = new QLabel("大于", &dialog);
		greaterAlarmLabel->setStyleSheet("QLabel {color: white; }");
		QLineEdit* greaterAlarmInput = new QLineEdit(&dialog);
		greaterAlarmInput->setStyleSheet("QLineEdit {color: white;}");
		greaterAlarmInput->setValidator(new QDoubleValidator(-1000, 10000, 2, greaterAlarmInput));
		//greaterAlarmInput->setReadOnly(true); // 设置为只读
		QLabel* lessAlarmLabel = new QLabel("或小于", &dialog);
		lessAlarmLabel->setStyleSheet("QLabel {color: white; }");
		QLineEdit* lessAlarmInput = new QLineEdit(&dialog);
		lessAlarmInput->setStyleSheet("QLineEdit {color: white;}");
		lessAlarmInput->setValidator(new QDoubleValidator(-1000, 10000, 2, lessAlarmInput));
		//lessAlarmInput->setReadOnly(true); // 设置为只读
		QVariantList alarmValue = settingDefaults["alarmValue"].toList();
		if (!alarmValue.isEmpty()) {
			greaterAlarmInput->setText(QString::number(alarmValue[0].toDouble(), 'f', 2));
			lessAlarmInput->setText(QString::number(alarmValue[1].toDouble(), 'f', 2)); // 更正此处
		}

		alarmInputs1.push_back(greaterAlarmInput);
		alarmInputs2.push_back(lessAlarmInput);
		alarmLayout->addWidget(alarmLabel);
		alarmLayout->addWidget(greaterAlarmLabel);
		alarmLayout->addWidget(greaterAlarmInput);
		alarmLayout->addWidget(lessAlarmLabel);
		alarmLayout->addWidget(lessAlarmInput);
		gridLayout->addLayout(alarmLayout, row + 3, 0, 1, 2);
	}


	// 设置网格布局的间距和边距
	gridLayout->setHorizontalSpacing(20);
	gridLayout->setVerticalSpacing(10);
	gridLayout->setContentsMargins(20, 20, 20, 20);

	// 设置对话框的样式
	dialog.setStyleSheet("QDialog { background-color: #f0f0f0; }"
		"QLabel { font-size: 12pt; }"
		"QLineEdit { border: 1px solid #999999; border-radius: 4px; padding: 2px; background-color: #ffffff; }"
		"QPushButton { border: 2px solid #4a76a8; border-radius: 6px; background-color: #5a8bbf; padding: 5px; font-size: 10pt; color: white; min-width: 80px; }"
		"QPushButton:hover { background-color: #6b9cd5; }"
		"QPushButton:pressed { background-color: #487aa1; }");

	// 其他布局和控件设置保持不变


	// 创建确定和取消按钮
	QPushButton *confirmButton = new QPushButton("确定", &dialog);
	QPushButton *cancelButton = new QPushButton("取消", &dialog);

	QObject::connect(confirmButton, &QPushButton::clicked, [&]() {
		QString selectedParentName = m_configLoader->getSelectedParentNames(); // 获取当前选中的父类名称
		bool shouldReplot = false; // 标记是否需要重绘图表

		for (int i = 0; i < settingLabels.size(); ++i) {
			QString settingName = settingLabels[i];
			QVariantList yAxisRange = { rangeInputs1[i]->text().toDouble(), rangeInputs2[i]->text().toDouble() };
			QVariantList warningValue = { warningInputs1[i]->text().toDouble(), warningInputs2[i]->text().toDouble() };
			QVariantList alarmValue = { alarmInputs1[i]->text().toDouble(), alarmInputs2[i]->text().toDouble() };

			m_configLoader->updateSetting(settingName, "yAxisRange", yAxisRange);
			m_configLoader->updateSetting(settingName, "warningValue", warningValue);
			m_configLoader->updateSetting(settingName, "alarmValue", alarmValue);

			if (settingName == selectedParentName) {
				// 更新图表的Y轴范围
				m_plot->setAxisScale(QwtPlot::yLeft, yAxisRange[0].toDouble(), yAxisRange[1].toDouble());
				shouldReplot = true; // 标记需要重绘图表
				qDebug() << "Setting Y-axis range for" << settingName << "to" << yAxisRange;
			}
		}
		m_configLoader->saveConfig("Config/Event.json");

		if (shouldReplot) {
			m_plot->replot(); // 如果需要，则重新绘制图表以应用更改
		}

		dialog.accept(); // 关闭对话框
	});
	// 设置按钮的布局
	QHBoxLayout *buttonLayout = new QHBoxLayout;
	buttonLayout->addStretch(); // 添加弹性空间，使按钮靠右对齐
	buttonLayout->addWidget(confirmButton);
	buttonLayout->addSpacing(10); // 在两个按钮之间添加10像素的间距
	buttonLayout->addWidget(cancelButton);


	//// 连接按钮的信号与槽
	//QObject::connect(confirmButton, &QPushButton::clicked, &dialog, &QDialog::accept);
	QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

	// 将按钮布局添加到网格布局的下方
	gridLayout->addLayout(buttonLayout, settingLabels.size() * 5, 0, 1, 2); // 调整行位置以适应新的布局

	// 设置网格布局的间距和边距
	gridLayout->setHorizontalSpacing(30); // 增加水平间距
	gridLayout->setVerticalSpacing(15); // 增加垂直间距
	gridLayout->setContentsMargins(20, 20, 20, 20); // 设置布局的边距

	scrollArea->setWidget(container);
	QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
	mainLayout->addWidget(scrollArea);
	mainLayout->setMargin(0);

	//dialog.setMaximumSize(900, 800);
	dialog.setMinimumSize(650, 500);


	//// 将网格布局添加到对话框
	//dialog.setLayout(gridLayout);
	dialog.adjustSize();
	dialog.exec();

}



//对齐度设置
void lpTendencyDataChart::AlignPBClicked()
{
	QDialog dialog(m_widget); // 使用当前widget作为父窗口
	dialog.setWindowTitle("对齐度设置");
	//dialog.resize(650, 500);
	//dialog.setFixedSize(dialog.size());
			// 设置字体大小
	QFont font = dialog.font();
	font.setPointSize(14); // 设置字体大小为14点
	dialog.setFont(font);

	QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

	QScrollArea *scrollArea = new QScrollArea(&dialog);
	scrollArea->setWidgetResizable(true);
	QWidget *container = new QWidget();
	QVBoxLayout *mainLayout = new QVBoxLayout(container);

	QTabWidget *tabWidget = new QTabWidget(container);
	QWidget *ceramicTab = new QWidget();
	QWidget *earTab = new QWidget();
	QWidget *plasmaTab = new QWidget();

	QGridLayout *ceramicLayout = new QGridLayout(ceramicTab);
	QGridLayout *earLayout = new QGridLayout(earTab);
	QGridLayout *plasmaLayout = new QGridLayout(plasmaTab);


	//QGridLayout *gridLayout = new QGridLayout(container);


	// 获取除了“对齐度”之外的所有子类名称
	QStringList subCategoryNames = m_configLoader->getAllCurveNamesExceptParent();


	// 创建左侧和右侧的单选按钮组
	QButtonGroup *leftGroup = new QButtonGroup(&dialog);
	QButtonGroup *rightGroup = new QButtonGroup(&dialog);


	//创建左侧和右侧的选项列表
	QStringList leftOptions;
	QStringList rightOptions;



	for (const QString &name : subCategoryNames)
	{
		if (name.contains("A") && !name.contains("居中度"))
		{
			leftOptions.append(name);
		}
		else if (name.contains("B") && !name.contains("居中度"))
		{
			rightOptions.append(name);
		}
	}

	// 添加左侧单选按钮
	for (int i = 0; i < leftOptions.size(); ++i) {
		QRadioButton *radioButton = new QRadioButton(leftOptions[i], &dialog);
		leftGroup->addButton(radioButton, i);
		if (leftOptions[i].contains("陶瓷区")) {
			ceramicLayout->addWidget(radioButton, i, 0);
		}
		else if (leftOptions[i].contains("极耳区")) {
			earLayout->addWidget(radioButton, i, 0);
		}
		else if (leftOptions[i].contains("电浆区")) {
			plasmaLayout->addWidget(radioButton, i, 0);
		}
	}

	// 添加右侧单选按钮
	for (int i = 0; i < rightOptions.size(); ++i) {
		QRadioButton *radioButton = new QRadioButton(rightOptions[i], &dialog);
		rightGroup->addButton(radioButton, i);
		if (rightOptions[i].contains("陶瓷区")) {
			ceramicLayout->addWidget(radioButton, i, 1);
		}
		else if (rightOptions[i].contains("极耳区")) {
			earLayout->addWidget(radioButton, i, 1);
		}
		else if (rightOptions[i].contains("电浆区")) {
			plasmaLayout->addWidget(radioButton, i, 1);
		}
	}


	tabWidget->addTab(ceramicTab, "陶瓷区");
	tabWidget->addTab(earTab, "极耳区");
	tabWidget->addTab(plasmaTab, "电浆区");

	mainLayout->addWidget(tabWidget);
	scrollArea->setWidget(container);
	dialogLayout->addWidget(scrollArea);
	mainLayout->setMargin(0);
	dialogLayout->setMargin(0);

	// 创建显示对齐度的文本框和标签
	QLabel *alignmentLabel = new QLabel("对齐度名称:", &dialog);
	QLineEdit *alignmentDisplay = new QLineEdit(&dialog);
	alignmentDisplay->setReadOnly(true);
	// 创建一个水平布局来包含标签和文本框
	QHBoxLayout *alignmentLayout = new QHBoxLayout;
	alignmentLayout->addWidget(alignmentLabel);
	alignmentLayout->addWidget(alignmentDisplay);
	dialogLayout->addLayout(alignmentLayout);

	// 创建删除，确定和取消按钮
	QPushButton *deleteButton = new QPushButton("删除", &dialog);
	QPushButton *confirmButton = new QPushButton("确定", &dialog);
	QPushButton *cancelButton = new QPushButton("取消", &dialog);


	// 设置按钮的布局
	QHBoxLayout *buttonLayout = new QHBoxLayout;
	buttonLayout->addStretch(); // 添加弹性空间，使按钮靠右对齐
	buttonLayout->addWidget(deleteButton);
	buttonLayout->addWidget(confirmButton);
	buttonLayout->addSpacing(10); // 在两个按钮之间添加10像素的间距
	buttonLayout->addWidget(cancelButton);
	dialogLayout->addLayout(buttonLayout);

	dialog.adjustSize();

	auto updateAlignmentDisplay = [&]() {

		int leftId = leftGroup->checkedId();
		int rightId = rightGroup->checkedId();

		if (leftId != -1 && rightId != -1) {
			QString leftOption = leftOptions[leftId];
			QString rightOption = rightOptions[rightId];

			//使用正则表达式提取前缀和后缀
			QRegularExpression re("^\\((.*?)\\)([A-Z])面(.*?)宽度$");
			QRegularExpressionMatch leftMatch = re.match(leftOption);
			QRegularExpressionMatch rightMatch = re.match(rightOption);

			if (leftMatch.hasMatch() && rightMatch.hasMatch())
			{
				QString commonPrefix = leftMatch.captured(1);//例如"（通道1）"
				QString rightPrefix = rightMatch.captured(1);
				// 检查左右选项的通道是否相同
				if (commonPrefix != rightPrefix) {
					QMessageBox::warning(nullptr, "错误", "左右两边选的通道必须相同！");
					alignmentDisplay->clear();

					return; // 提前退出函数
				}


				QString leftSuffix = leftMatch.captured(2);//例如" A "
				QString rightSuffix = rightMatch.captured(2);//例如 " B "
				QString commonPostfix = leftMatch.captured(3);//例如" 陶瓷区 "

				QString alignment = QString("(%1)%2/%3面%4对齐度").arg(commonPrefix, leftSuffix, rightSuffix, commonPostfix);
				alignmentDisplay->setText(alignment);
			}
			else
			{

				alignmentDisplay->clear();
			}
		}
		else
		{
			alignmentDisplay->clear();
		}

	};

	// 连接单选按钮的信号与槽
	QObject::connect(leftGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), updateAlignmentDisplay);
	QObject::connect(rightGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), updateAlignmentDisplay);

	// 连接按钮的信号与槽
	QObject::connect(confirmButton, &QPushButton::clicked, [&]() {
		int leftId = leftGroup->checkedId();
		int rightId = rightGroup->checkedId();

		if (leftId != -1 && rightId != -1) {
			QString leftOption = leftOptions[leftId];
			QString rightOption = rightOptions[rightId];


			// 检查两个选项是否属于同一类别
			if (!isSameCategory(leftOption, rightOption)) {
				QMessageBox::warning(&dialog, "错误", "左右两侧的选项必须属于同一类别。");
				return;
			}

			//使用正则表达式提取前缀和后缀
			QRegularExpression re("^\\((.*?)\\)([A-Z])面(.*?)宽度$");
			QRegularExpressionMatch leftMatch = re.match(leftOption);
			QRegularExpressionMatch rightMatch = re.match(rightOption);

			QString alignment;
			if (leftMatch.hasMatch() && rightMatch.hasMatch())
			{
				QString rightPrefix = rightMatch.captured(1);
				QString commonPrefix = leftMatch.captured(1);//例如"（通道1）"
				if (commonPrefix != rightPrefix) {
					QMessageBox::warning(nullptr, "错误", "左右两边选的通道必须相同！");
					alignmentDisplay->clear();

					return; // 提前退出函数
				}
				QString leftSuffix = leftMatch.captured(2);//例如" A "
				QString rightSuffix = rightMatch.captured(2);//例如 " B "
				QString commonPostfix = leftMatch.captured(3);//例如" 陶瓷区 "

				alignment = QString("(%1)%2/%3面%4对齐度").arg(commonPrefix, leftSuffix, rightSuffix, commonPostfix);

			}
			else
			{
				QMessageBox::warning(&dialog, "格式错误", "选项不正确");
				return;
			}
			//QString alignment = QString("%1/%2对齐度").arg(leftOption, rightOption);
			bool display = true;
			// 保存对齐度设置
			qDebug() << "Selected alignment:" << alignment;



			// 根据对齐度名称确定应该添加到哪个父类别
			QString parentCategory;
			bool shouldEmit = true;//判断是否要更新当前图例界面
			if (alignment.contains("陶瓷区")) {
				parentCategory = "陶瓷区对齐度";
			}
			else if (alignment.contains("极耳区")) {
				parentCategory = "极耳区对齐度";
			}
			else if (alignment.contains("电浆区")) {
				parentCategory = "电浆区对齐度";
			}
			else {
				// 默认分类或错误处理
				parentCategory = "其他对齐度";
			}

			// 更新配置文件或应用设置
			m_configLoader->addNewChildToCategory(parentCategory, alignment, display, &shouldEmit);

			if (shouldEmit)
			{
				// 同步更新趋势勾选指标和趋势图的曲线名称
				emit m_configLoader->curveDisplayChanged(alignment, true);

				dialog.accept(); // 关闭对话框
			}

		}

	});

	QObject::connect(deleteButton, &QPushButton::clicked, [alignmentDisplay, this, &dialog]() {
		QString selectedName = alignmentDisplay->text(); // 从QLineEdit获取当前选中的子类名称
		if (selectedName.isEmpty()) {
			QMessageBox::warning(&dialog, "删除错误", "没有选中任何对齐度名称，请选择一个对齐度名称后再尝试删除。");
			return;
		}

		// 检查是否存在该名称
		QStringList allCurveNames = m_configLoader->getAllCurveNames();
		if (!allCurveNames.contains(selectedName)) {
			QMessageBox::warning(&dialog, "删除错误", "要删除的对齐度名称不存在，请选择一个有效的对齐度名称后再尝试删除。");
			return;
		}

		// 执行删除操作
		QString parentCategory = determineParentCategory(selectedName);
		m_configLoader->removeChildFromCategory(parentCategory, selectedName);
		QMessageBox::information(&dialog, "删除成功", "已成功删除。");
		dialog.accept(); // 关闭对话框
	});


	QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

	dialog.exec();

}

QString lpTendencyDataChart::determineParentCategory(const QString& alignmentName) {
	if (alignmentName.contains("陶瓷区")) {
		return "陶瓷区对齐度";
	}
	else if (alignmentName.contains("极耳区")) {
		return "极耳区对齐度";
	}
	else if (alignmentName.contains("电浆区")) {
		return "电浆区对齐度";
	}
	else {
		return "其他对齐度";
	}
}

bool lpTendencyDataChart::isSameCategory(const QString & option1, const QString & option2)
{
	QStringList categories = { "陶瓷", "电浆", "极耳区" };
	for (const QString &category : categories) {
		if (option1.contains(category) && option2.contains(category)) {
			return true;
		}
	}
	return false;
}

QColor colorFromName(const QString &name) {
	// 使用Qt的qHash函数生成一个基于字符串的哈希值
	quint32 hashValue = qHash(name);

	// 使用哈希值来生成颜色
	int h = (hashValue % 360); // 色相值在0到359之间
	int s = 200 + (hashValue % 55); // 饱和度在200到255之间
	int v = 150 + (hashValue % 105); // 明度在150到255之间

	return QColor::fromHsv(h, s, v);
}


void lpTendencyDataChart::addCurve(const QString &curveName) {
	QwtPlotCurve *curve = new QwtPlotCurve(curveName);
	curve->setTitle(curveName); // 设置曲线的标题，这将在图例中显示
		// 设置曲线标题的字体大小
	QwtText title(curveName);
	QFont font;
	font.setPointSize(12); // 设置字体大小为12点
	title.setFont(font);
	curve->setTitle(title);


	QColor color = colorFromName(curveName);
	curve->setPen(color, 2); // 设置曲线颜色和宽度

		// 启用抗锯齿
	curve->setRenderHint(QwtPlotItem::RenderAntialiased);

	//// 使用样条曲线插值
	//curve->setCurveAttribute(QwtPlotCurve::Fitted, true);


	curve->attach(m_plot);
	m_curves.append(curve);
	m_xDataMap[curveName] = QVector<double>(); // 初始化数据存储
	m_yDataMap[curveName] = QVector<double>();
	// 根据curveNames列表设置曲线的可见性和图例显示
	bool isVisible = m_curveNames.contains(curveName);
	curve->setVisible(isVisible); // 根据是否存在于curveNames中设置可见性
	curve->setItemAttribute(QwtPlotItem::Legend, isVisible); // 同样设置图例的显示

	m_plot->replot(); // 重绘图表以应用更改
}

void lpTendencyDataChart::onLegendClicked(const QVariant &itemInfo, int index) {
	QwtPlotItem *clickedItem = m_plot->infoToItem(itemInfo);
	if (!clickedItem) return;

	// 首先重置所有曲线到初始透明度状态
	resetCurvesOpacity();

	// 然后设置未被点击曲线的透明度
	for (auto &curve : m_curves) {
		if (curve != clickedItem) {
			QColor color = curve->pen().color();
			color.setAlpha(50); // 设置为半透明
			curve->setPen(QPen(color, 2));
			curve->setRenderHint(QwtPlotItem::RenderAntialiased);
		}
	}

	m_plot->replot(); // 重绘图表以应用更改
}


void lpTendencyDataChart::resetCurvesOpacity() {
	for (auto &curve : m_curves) {
		QColor color = curve->pen().color();
		color.setAlpha(255); // 设置为完全不透明
		curve->setPen(QPen(color, 2));
		curve->setRenderHint(QwtPlotItem::RenderAntialiased);
	}
}

void lpTendencyDataChart::installEventFilters() {
	m_plot->canvas()->installEventFilter(this);
}


bool lpTendencyDataChart::eventFilter(QObject *watched, QEvent *event) {
	if (watched == m_plot->canvas()) {
		switch (event->type()) {
		case QEvent::MouseButtonPress: {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
			if (mouseEvent->button() == Qt::LeftButton) {
				m_dragStartPosition = mouseEvent->pos();
				m_xMinCurrent = m_plot->axisScaleDiv(QwtPlot::xBottom).lowerBound();
				m_xMaxCurrent = m_plot->axisScaleDiv(QwtPlot::xBottom).upperBound();
				m_isDragging = true;


				m_plot->replot(); // 重绘图表以应用更改
				//isViewingHistory = true; // 开始查看历史
				return true;
			}
			else if (mouseEvent->button() == Qt::RightButton) {
				// 当用户右键点击时，退出历史查看模式
				m_isViewingHistory = false;
				resetCurvesOpacity();
				double xMax = m_maxReceivedX;
				if (xMax <= 0)
				{
					xMax = 50;
				}
				double xMin = xMax - 50; // 假设显示范围是50
				if (xMin < 0) xMin = 0; // 确保xMin不小于0
				m_plot->setAxisScale(QwtPlot::xBottom, xMin, xMax);
				m_plot->replot(); // 重绘图表以应用更改
				return true;

			}
			break;
		}
		case QEvent::MouseMove: {
			if (m_isDragging) {
				QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
				int dx = mouseEvent->pos().x() - m_dragStartPosition.x();
				m_isViewingHistory = true;
				double shift = (m_xMaxCurrent - m_xMinCurrent) * dx / m_plot->canvas()->width();
				double newMin = m_xMinCurrent - shift;
				double newMax = m_xMaxCurrent - shift;

				// 确保新的x轴范围不小于0
				if (newMin < m_lastResetX) {
					newMin = m_lastResetX;
					newMax = newMin + (m_xMaxCurrent - m_xMinCurrent); // 保持区间长度不变
				}

				m_plot->setAxisScale(QwtPlot::xBottom, newMin, newMax);
				m_plot->replot();
				//updateSliderPosition();// 确保滑动条位置更新
				return true;
			}
			break;
		}
		case QEvent::MouseButtonRelease: {
			if (m_isDragging) {
				m_isDragging = false;
				//isViewingHistory = false; // 结束查看历史
				return true;
			}
			break;
		}
		default:
			break;
		}
	}
	return QObject::eventFilter(watched, event);
}


void lpTendencyDataChart::onCurveDisplayChanged(const QString &curveName, bool display) {
	for (auto &curve : m_curves) {
		if (curve->title().text() == curveName) {
			curve->setVisible(display);  // 设置曲线的可见性
			curve->setItemAttribute(QwtPlotItem::Legend, display);  // 控制是否在图例中显示
			m_plot->replot();  // 重绘图表以应用更改
			return;  // 找到曲线后返回
		}
	}

	// 如果曲线不存在且需要显示，则添加新曲线
	if (display) {
		addCurve(curveName);  // 添加曲线
		// 更新ChartUpdaterThread中的曲线名称列表
		QStringList updatedCurveNames;
		for (auto &curve : m_curves) {
			updatedCurveNames.append(curve->title().text());
		}
		//updaterThread->updateCurveNames(updatedCurveNames);
		m_curveNames.append(curveName);  // 添加曲线名称到curveNames列表，用于其他同步操作
		m_plot->replot();  // 重绘图表
	}
}


void lpTendencyDataChart::updateYAxisRange(const QVariantList &yAxisRange) {
	if (yAxisRange.size() == 2) {
		double minY = yAxisRange[0].toDouble();
		double maxY = yAxisRange[1].toDouble();
		m_plot->setAxisScale(QwtPlot::yLeft, minY, maxY);
		m_plot->replot();
	}
}

void lpTendencyDataChart::updateWarningValue(const QVariantList &warningValue) {
	if (warningValue.size() == 2) {
		m_warningValueLower = warningValue[0].toDouble();
		m_warningValueUpper = warningValue[1].toDouble();
	}
}

void lpTendencyDataChart::updateAlarmValue(const QVariantList &alarmValue) {
	if (alarmValue.size() == 2) {
		m_alarmValueLower = alarmValue[0].toDouble();
		m_alarmValueUpper = alarmValue[1].toDouble();
	}
}

void lpTendencyDataChart::onSliderValueChanged(int value)
{
	m_isViewingHistory = true; // 设置为查看历史数据模式
	double xMin = value;
	double xMax = xMin + 50;  // 假设一次显示50m的数据

	//// 确保滑块移动不会超出最新的1000米数据范围
	//if (xMin < lastResetX) {
	//	xMin = lastResetX;
	//	xMax = xMin + 50;
	//	m_slider->setValue(static_cast<int>(xMin)); // 重新设置滑块位置
	//}

	m_plot->setAxisScale(QwtPlot::xBottom, xMin, xMax);
	m_plot->replot();
}


void lpTendencyDataChart::updateSliderPosition() {
	int xMinCurrent = static_cast<int>(m_plot->axisScaleDiv(QwtPlot::xBottom).lowerBound());
	int xMaxCurrent = static_cast<int>(m_plot->axisScaleDiv(QwtPlot::xBottom).upperBound());

	// 设置滑块的范围为最新的1000米数据范围
	m_slider->setMinimum(static_cast<int>(m_lastResetX));
	m_slider->setMaximum(xMaxCurrent - 50); // 假设显示范围是50，确保滑块不会超出当前显示的最大范围

	// 设置滑块的当前值为x轴的最小值，确保与图表同步
	m_slider->setValue(xMinCurrent);
}

void lpTendencyDataChart::clearChart()
{

	for (auto &curve : m_curves) {
		m_xDataMap[curve->title().text()].clear();
		m_yDataMap[curve->title().text()].clear();
		curve->setSamples(m_xDataMap[curve->title().text()], m_yDataMap[curve->title().text()]);
	}
	m_lastResetX = 0;
	m_maxReceivedX = 0;
	// 重置x轴和y轴的范围
	m_plot->setAxisScale(QwtPlot::xBottom, 0, 50); // 重置X轴的范围为0-50

	// 重新绘制图表
	m_plot->replot();
}

