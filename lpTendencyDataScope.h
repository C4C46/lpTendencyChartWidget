﻿#pragma once

#include <QObject>
#include <QTableWidget>
#include <QList>
#include <QHeaderView>
#include <QEvent>
#include <QScrollBar>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QTimer>
#include <qDebug>
#include <QThread>
#include "lpTendencyChartUpdateThread.h"


class lpTendencyDataScope : public QObject
{
	Q_OBJECT


public:
	lpTendencyDataScope(QTableWidget* tableWidget, QObject* parent = nullptr);
	lpTendencyDataScope::~lpTendencyDataScope();
	void setColumnNames(const QStringList &names);//表格表头加载布局
	void addData(const QString & curveName, double x, double y, const QVariantList & warningValue, const QVariantList & alarmValue);//添加数据缓存
	void saveTableSettings(const QStringList& identifiers);//保存表格设置的列宽和列排序位置
	void loadTableSettings(const QStringList& identifiers);//加载表格设置的列宽和列排序位置
	void loadSettingsFromFile();//加载对应列的位置信息
	void saveSettingsToFile();//保存对应列的位置信息


signals:
	void sgDataCache(QMap<QString, QList<QPair<double, QPair<double, QVariantList>>>> dataCache);

public slots:
	void onSendData(QString DataName, double xData, double yData, QVariantList warningValue, QVariantList AlarmingValue);//更新表格中的数据内容

protected:
	bool eventFilter(QObject *obj, QEvent *event);//表格功能设置

private:
	QTableWidget* data_tableWidget;


	QMap<double, int> m_xDataToRowMap;	// 使用哈希表来存储xData与行号的映射，减少查找时间
	QStringList m_columnNames;//存储列名
	QMap<QString, QVariant> m_settingsCache;  // 缓存设置
	bool m_autoScrollEnabled = true;  // 默认启用自动滚动
	int reloadCount = 0; // 记录表格重新加载次数的成员变量
	QMap<QString, QList<QPair<double, QPair<double, QVariantList>>>> m_dataCache; // 数据缓存
	lpTendencyChartUpdateThread* m_UpdateThread;//表格数据处理工作线程
	QThread *m_thread;
	bool m_enableDataLoading = true;//先显示界面再加载数据

};

