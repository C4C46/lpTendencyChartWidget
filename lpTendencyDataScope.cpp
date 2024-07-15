
#include "lpTendencyDataScope.h"
#pragma execution_character_set("utf-8")

lpTendencyDataScope::lpTendencyDataScope(QTableWidget* tableWidget, QObject* parent)
	: QObject(parent), data_tableWidget(tableWidget) {
	// 初始化表格设置
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


}



lpTendencyDataScope::~lpTendencyDataScope()
{
	if (m_thread && m_thread->isRunning()) {
		m_thread->quit();
		m_thread->wait();
	}
	delete m_UpdateThread;

	//updateTimer->stop();
}



void lpTendencyDataScope::setColumnNames(const QStringList & names)
{
	m_enableDataLoading = false;
	saveTableSettings(m_columnNames);


	m_columnNames = names;

	data_tableWidget->setColumnCount(m_columnNames.size() + 1); // 加1是因为第一列是X值
	QStringList headers = QStringList() << "位置（m）";
	headers.append(m_columnNames);
	data_tableWidget->setHorizontalHeaderLabels(headers);

	// 隐藏行号
	data_tableWidget->verticalHeader()->setVisible(false);

	data_tableWidget->setColumnWidth(0, 100); // 设置第一列的固定宽度
	data_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // 第一列宽度固定

	QFontMetrics metrics(data_tableWidget->font());
	int minWidth = 220; // 最小宽度
	for (int i = 1; i < headers.size(); ++i) {
		int width = metrics.width(headers[i]) + 20; // 加20像素留白
		width = qMax(width, minWidth); // 确保不小于最小宽度
		data_tableWidget->setColumnWidth(i, width);
		data_tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Interactive);

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

	loadTableSettings(m_columnNames);
	m_enableDataLoading = true;  
}




void lpTendencyDataScope::addData(const QString &curveName, double x, double y, const QVariantList &warningValue, const QVariantList &alarmValue)

{
	if (!m_xDataToRowMap.contains(x)) {
		// 如果不存在，添加到缓存中
		m_dataCache[curveName].append(qMakePair(x, qMakePair(y, QVariantList{ warningValue, alarmValue })));
		emit sgDataCache(m_dataCache);
	}

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

	QTableWidgetItem *item = new QTableWidgetItem(QString::number(yData));
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
		data_tableWidget->scrollToBottom();
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

void lpTendencyDataScope::saveTableSettings(const QStringList& identifiers) {
	QString identifier = identifiers.join("_");
	//qDebug() << "Saving settings for identifier:" << identifier;
	m_settingsCache["DataTableSettings_" + identifier + "_ColumnWidths"].clear();
	m_settingsCache["DataTableSettings_" + identifier + "_ColumnPositions"].clear();

	QVariantList widths, positions;
	for (int i = 0; i < data_tableWidget->columnCount(); ++i) {
		widths << data_tableWidget->columnWidth(i);
		positions << data_tableWidget->horizontalHeader()->visualIndex(i);
	}
	m_settingsCache["DataTableSettings_" + identifier + "_ColumnWidths"] = widths;
	m_settingsCache["DataTableSettings_" + identifier + "_ColumnPositions"] = positions;
}

void lpTendencyDataScope::loadTableSettings(const QStringList& identifiers) {
	QString identifier = identifiers.join("_");
	//qDebug() << "Loading settings for identifier:" << identifier;
	QVariantList widths = m_settingsCache["DataTableSettings_" + identifier + "_ColumnWidths"].toList();
	QVariantList positions = m_settingsCache["DataTableSettings_" + identifier + "_ColumnPositions"].toList();

	for (int i = 0; i < widths.size(); ++i) {
		data_tableWidget->setColumnWidth(i, widths[i].toInt());
	}
	for (int i = 0; i < positions.size(); ++i) {
		data_tableWidget->horizontalHeader()->moveSection(data_tableWidget->horizontalHeader()->visualIndex(i), positions[i].toInt());
	}

}

void lpTendencyDataScope::loadSettingsFromFile() {
	QString settingsFile = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
	QSettings settings(settingsFile, QSettings::IniFormat);
	settings.beginGroup("DataTableSettings");
	m_settingsCache = settings.value("SettingsCache").toMap();
	m_columnNames = settings.value("SelectedSubclassNames").toStringList();  // 加载列名

	settings.endGroup();
	qDebug() << "Settings loaded from file:" << settingsFile;
	qDebug() << "SettingsCache size:" << m_settingsCache.size();

	loadTableSettings(m_columnNames);

}

void lpTendencyDataScope::saveSettingsToFile() {
	QString settingsFile = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
	QSettings settings(settingsFile, QSettings::IniFormat);
	settings.beginGroup("DataTableSettings");

	settings.setValue("SelectedSubclassNames", m_columnNames);
	settings.setValue("SettingsCache", m_settingsCache);
	settings.endGroup();
	qDebug() << "Settings saved to file:" << settingsFile;
	qDebug() << "SettingsCache size:" << m_settingsCache.size();
}


