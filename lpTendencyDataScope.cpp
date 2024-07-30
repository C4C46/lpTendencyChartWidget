
#include "lpTendencyDataScope.h"
#pragma execution_character_set("utf-8")

lpTendencyDataScope::lpTendencyDataScope(QTableWidget* tableWidget, QObject* parent)
	: QObject(parent), data_tableWidget(tableWidget) {

	//loadColumnConfig();
	// 初始化表格设置
	loadLithiumRowsConfig();

	data_tableWidget->setColumnCount(0); // 初始列数设置为0
	data_tableWidget->setHorizontalHeaderLabels(QStringList() << "X Value"); // 设置第一列标题
	data_tableWidget->verticalHeader()->setVisible(false);
	data_tableWidget->verticalScrollBar()->installEventFilter(this);  // 安装事件过滤器

	data_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	data_tableWidget->setSelectionMode(QAbstractItemView::NoSelection);

	// 允许列拖放重排
	data_tableWidget->horizontalHeader()->setSectionsMovable(true);
	data_tableWidget->horizontalHeader()->setDragEnabled(true);
	data_tableWidget->horizontalHeader()->setDragDropMode(QAbstractItemView::InternalMove);
	data_tableWidget->horizontalHeader()->setDropIndicatorShown(true);

	QFont font = data_tableWidget->font();
	font.setPointSize(12);
	data_tableWidget->setFont(font);


	qRegisterMetaType<QMap<QString, QList<QPair<double, QPair<double, QVariantList>>>>>("QMap<QString, QList<QPair<double, QPair<double, QVariantList>>>>");
	m_UpdateThread = new lpTendencyChartUpdateThread();
	m_thread = new QThread(this);
	m_UpdateThread->moveToThread(m_thread);

	connect(this, &lpTendencyDataScope::sgDataCache, m_UpdateThread, &lpTendencyChartUpdateThread::onDataCache);
	connect(m_UpdateThread, &lpTendencyChartUpdateThread::sgSendData, this, &lpTendencyDataScope::onSendData);
	m_thread->start();
	//initializeRows(0, 10000);


}



lpTendencyDataScope::~lpTendencyDataScope()
{
	if (m_thread && m_thread->isRunning()) {
		m_thread->quit();
		m_thread->wait();
	}
	delete m_UpdateThread;

	//saveColumnConfig();
	//updateTimer->stop();
}



void lpTendencyDataScope::setColumnNames(const QStringList & names)
{
	m_enableDataLoading = false;/*
	saveTableSettings(m_columnNames);*/

	m_columnNames = names;

	data_tableWidget->setColumnCount(m_columnNames.size() + 1); // 加1是因为第一列是X值
	QStringList headers = QStringList() << "位置（m）";
	// 遍历原始名称列表，进行表头的简化处理
	for (const QString &name : m_columnNames) {
		QRegularExpression reWidth("\\(通道:(\\d+)\\)([AB])面(.+?)区宽度");
		QRegularExpression reAlignment("\\(通道:(\\d+)\\)A/B面(.+?)区对齐度");
		QRegularExpressionMatch matchWidth = reWidth.match(name);
		QRegularExpressionMatch matchAlignment = reAlignment.match(name);

		if (matchWidth.hasMatch()) {
			QString simplifiedHeader = QString("(%1)%2\n%3").arg(matchWidth.captured(1), matchWidth.captured(2), matchWidth.captured(3));
			headers.append(simplifiedHeader);
		}
		else if (matchAlignment.hasMatch()) {
			QString simplifiedHeader = QString("(%1)A/B\n%3").arg(matchAlignment.captured(1), matchAlignment.captured(2), matchAlignment.captured(3));
			headers.append(simplifiedHeader);
		}
		else {
			headers.append(name); // 如果不匹配，直接使用原始名称
		}
	}

	//headers.append(m_columnNames);
	data_tableWidget->setHorizontalHeaderLabels(headers);

	// 隐藏行号
	data_tableWidget->verticalHeader()->setVisible(false);

	// 设置列宽
	QFontMetrics metrics(data_tableWidget->font());
	int minWidth = 100; // 最小宽度
	for (int i = 0; i < headers.size(); ++i) {
		int width = metrics.width(headers[i]) + 20; // 加20像素留白
		width = qMax(width, minWidth); // 确保不小于最小宽度
		data_tableWidget->setColumnWidth(i, width);
		if (i < headers.size() - 1) {
			data_tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Interactive);
		}
		else {
			data_tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
		}
	}

	// 设置工具提示显示完整的列名称
	for (int i = 0; i < headers.size(); ++i) {
		QTableWidgetItem* headerItem = data_tableWidget->horizontalHeaderItem(i);
		if (!headerItem) {
			headerItem = new QTableWidgetItem();
			data_tableWidget->setHorizontalHeaderItem(i, headerItem);
		}
		headerItem->setToolTip(headers[i]);
	}

	//loadTableSettings(m_columnNames);
	m_enableDataLoading = true;  
}

void lpTendencyDataScope::setColumnVisibility(const QString &curveName, bool visible) {
	int columnIndex = m_columnNames.indexOf(curveName) + 1; // 加1是因为第一列是X值
	if (columnIndex > 0) {
		if (!visible) {
			// 在隐藏前捕获宽度
			columnWidths[columnIndex] = data_tableWidget->columnWidth(columnIndex);
		}
		data_tableWidget->setColumnHidden(columnIndex, !visible);
		if (visible && columnWidths.contains(columnIndex)) {
			// 如果列重新显示，恢复之前的宽度
			data_tableWidget->setColumnWidth(columnIndex, columnWidths[columnIndex]);
		}
	}
}


void lpTendencyDataScope::initializeRows(int startM, int endM) {
	if (startM == 0) {
		startM = m_stepM; // 避免m=0的行
	}
	int numRows = (endM - startM) / m_stepM + 1;
	int currentRowCount = data_tableWidget->rowCount();
	data_tableWidget->setRowCount(currentRowCount + numRows);
	for (int i = 0; i < numRows; ++i) {
		int meter = startM + i * m_stepM;
		QTableWidgetItem* item = new QTableWidgetItem(QString::number(meter));
		data_tableWidget->setItem(currentRowCount + i, 0, item); // 设置每行的X值
		m_xDataToRowMap[meter] = currentRowCount + i; // 更新映射表
	}
}


void lpTendencyDataScope::addData(const QString &curveName, double x, double y, const QVariantList &warningValue, const QVariantList &alarmValue)

{


	// 检查是否达到新的1000米阈值
	if (static_cast<int>(x) - lastClearedMeter >= METER_THRESHOLD) {
		clearTable(); // 清空表格
		lastClearedMeter = static_cast<int>(x); // 更新上次清空的米数
		maxMeter = lastClearedMeter; // 重置maxMeter为当前x值
	}

	// 检查是否需要添加新的行
	if (x >= maxMeter) {
		int newStartM = maxMeter + m_stepM;
		int newEndM = newStartM + (m_stepM * (m_numColumns - 1)); // 根据步长和列数计算新的结束米数
		initializeRows(newStartM, newEndM); // 添加新的行
		maxMeter = newEndM; // 更新maxMeter为新的结束米数
	}


	int existingRow = m_xDataToRowMap.value(static_cast<int>(x), -1);
	if (existingRow == -1) return; // 如果没有找到对应的行，直接返回

		// 如果不存在，添加到缓存中
		m_dataCache[curveName].append(qMakePair(x, qMakePair(y, QVariantList{ warningValue, alarmValue })));
		emit sgDataCache(m_dataCache);
	

}


//数据更新
void lpTendencyDataScope::onSendData(QString DataName, double xData, double yData, QVariantList warningValue, QVariantList AlarmingValue)
{

	if (!m_enableDataLoading) return;

	int columnIndex = m_columnNames.indexOf(DataName) + 1;
	if (columnIndex <= 0) return;

	int existingRow = m_xDataToRowMap.value(xData, -1);
	data_tableWidget->setUpdatesEnabled(false);
	if (existingRow == -1) {
		existingRow = data_tableWidget->rowCount();
		data_tableWidget->insertRow(existingRow);
		data_tableWidget->setItem(existingRow, 0, new QTableWidgetItem(QString::number(xData)));
		m_xDataToRowMap[xData] = existingRow; // 更新映射表
	}

	// 清除之前所有行的位置列的光标标记
	for (int i = 0; i < data_tableWidget->rowCount(); ++i) {
		QTableWidgetItem* item = data_tableWidget->item(i, 0);
		if (item) {
			item->setBackground(Qt::white); // 重置背景色
		}
	}
	QTableWidgetItem* newItem = data_tableWidget->item(existingRow, 0);
	if (!newItem) {
		newItem = new QTableWidgetItem(QString::number(xData));
		data_tableWidget->setItem(existingRow, 0, newItem);
	}
	newItem->setBackground(Qt::yellow); // 设置特殊背景色以突出显示

	QTableWidgetItem *item = new QTableWidgetItem(QString::number(yData));
	item->setTextAlignment(Qt::AlignCenter);  // 设置文本居中
	// 设置背景颜色根据警告和报警值
	if (!AlarmingValue.isEmpty() && (yData > AlarmingValue[0].toDouble() || yData < AlarmingValue[1].toDouble())) {
		item->setBackground(Qt::red);
	}
	else if (!warningValue.isEmpty() && (yData > warningValue[0].toDouble() || yData < warningValue[1].toDouble())) {
		item->setBackground(QColor(255, 165, 0));
	}
	else {
		item->setBackground(Qt::white);
	}

	data_tableWidget->setItem(existingRow, columnIndex, item);
	data_tableWidget->setUpdatesEnabled(true);

	if (m_autoScrollEnabled) {
		data_tableWidget->scrollToItem(newItem, QAbstractItemView::PositionAtCenter); // 确保新行位于视图中心
	}
	m_dataCache.clear(); // 清空缓存

}




bool lpTendencyDataScope::eventFilter(QObject *obj, QEvent *event) {
	if (obj == data_tableWidget->verticalScrollBar()) {
		if (event->type() == QEvent::MouseButtonPress) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->button() == Qt::LeftButton) {
				// 鼠标左键按下，停止自动滚动
				m_autoScrollEnabled = false;
			}
			else if (mouseEvent->button() == Qt::RightButton) {
				// 鼠标右键按下，恢复自动滚动
				m_autoScrollEnabled = true;
				data_tableWidget->scrollToBottom();
				return true; // 消费掉此事件，防止进一步处理
			}
		}
		else if (event->type() == QEvent::Wheel) {
			QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
			// 检查是否滚动到底部
			if (data_tableWidget->verticalScrollBar()->value() == data_tableWidget->verticalScrollBar()->maximum()) {
				m_autoScrollEnabled = true;
				data_tableWidget->scrollToBottom();
			}
			else {
				m_autoScrollEnabled = false;
			}
		}
		else if (event->type() == QEvent::ContextMenu) {
			// 忽略上下文菜单事件
			return true;
		}
	}
	return QObject::eventFilter(obj, event);
}



void lpTendencyDataScope::saveColumnConfig() {

    QString configPath = QCoreApplication::applicationDirPath() + "/setting.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("ColumnConfig");

    QStringList columnOrder;
    for (int i = 0; i < data_tableWidget->horizontalHeader()->count(); ++i) {
        int logicalIndex = data_tableWidget->horizontalHeader()->logicalIndex(i);
        columnOrder.append(QString::number(logicalIndex));
		int defaultWidth = (i == 0) ? 100 : 200;
        int width = columnWidths.value(logicalIndex, defaultWidth); // 使用columnWidths获取宽度，如果不存在则默认为200

        qDebug() << "Saving width for column" << logicalIndex << ": " << width;
        settings.setValue(QString("Width_%1").arg(logicalIndex), width);
    }
	settings.setValue("Order", columnOrder.join(","));
	settings.endGroup();
}

void lpTendencyDataScope::loadColumnConfig() {
	QString configPath = QCoreApplication::applicationDirPath() + "/setting.ini";
	QSettings settings(configPath, QSettings::IniFormat);
	settings.beginGroup("ColumnConfig");
	QString order = settings.value("Order").toString();
	QStringList columnOrder = order.split(",");

	if (columnOrder.size() == data_tableWidget->horizontalHeader()->count()) {
		for (int i = 0; i < columnOrder.size(); ++i) {
			int logicalIndex = columnOrder[i].toInt();
			int width = settings.value(QString("Width_%1").arg(logicalIndex), 100).toInt();
			int currentVisualIndex = data_tableWidget->horizontalHeader()->visualIndex(logicalIndex);
			data_tableWidget->horizontalHeader()->moveSection(currentVisualIndex, i);
			data_tableWidget->setColumnWidth(logicalIndex, width);
		}
	}

	settings.endGroup();
}

void lpTendencyDataScope::loadLithiumRowsConfig() {
	QString configpath = QCoreApplication::applicationDirPath() + "/config/LithiumRows.json";
	QFile configFile(configpath);
	if (!configFile.open(QIODevice::ReadOnly)) {
		qWarning("Cannot open config file for reading.");
		m_stepM = 1000; // 默认步长
		m_numColumns = 10; // 默认列数
		return;
	}
	QByteArray data = configFile.readAll();
	QJsonDocument doc(QJsonDocument::fromJson(data));
	QJsonObject obj = doc.object();

	m_stepM = obj.value("stepM").toInt(1000); // 默认步长1000
	m_numColumns = obj.value("numColumns").toInt(10);
}

void lpTendencyDataScope::clearTable()
{
    data_tableWidget->setUpdatesEnabled(false);
    data_tableWidget->model()->removeRows(0, data_tableWidget->rowCount()); 
    m_xDataToRowMap.clear();
    m_dataCache.clear(); 
    data_tableWidget->setUpdatesEnabled(true); 
}
