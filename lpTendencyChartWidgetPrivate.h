﻿#pragma once
#include "lpTendencyChartWidget.h"
#include "ui_lpTendencyChartWidget.h"
#include "lpTendencyChartConfig.h"
#include "lpTendencyChartUpdateThread.h"
#include "lpTendencyDataScope.h"
#include "lpTendencyDataChart.h"
#include "lpTendencyManageThread.h"
#include <QThread>
#include <QInputDialog>
#include <QScrollArea>

class lpTendencyChartWidgetPrivate : public QObject
{
	Q_OBJECT
public:
	lpTendencyChartWidgetPrivate(lpTendencyChartWidget *parent);
	~lpTendencyChartWidgetPrivate();
	void init();
	void handleIntervalPBClicked();//参数设置
	void toggleTableVisibility();//趋势勾选指标控制是否隐藏
	void AlignPBClicked();//对齐度设置

	QWidget* getTopWidget();  // 提供访问 top_widget 的接口
	QWidget* getDownWidget();// 提供访问 down_widget 的接口

signals:
	void sgUpdateDataLithium(LithiumChannelRegionInfo_Tag * tag);
	void sgUpdateData(const QString &curveName, double x, double y);
	void sgClearScope();
	void sgClearChart();
	void sgChartRecipe(QString fileName, QString obj);
	void sgScopeRecipe(QString fileName, QString obj);

public slots:
	void updateDataScope(const QString &curveName, double x, double y);//接收数据更新数据表格
	void updateDataChart(const QString &curveName, double x, double y);
    void onChangeChartNumClicked();
    void setChartCount(int count);
    void setupCharts();
    QStringList filterCurveNamesByChartIndex(int index);
    //接收数据更新趋势图
private:
	Ui::lpTendencyChartWidget ui;
	lpTendencyChartConfig *m_ChartConfig{nullptr};
	lpTendencyDataScope *m_dataScope{nullptr};
	//lpTendencyDataChart *m_dataChart{nullptr};
	lpTendencyManageThread *m_manageThread{ nullptr };
	QThread *m_thread{ nullptr };

    QList<lpTendencyDataChart *> m_dataCharts; // 存储所有图表的列表
    int m_numCharts; // 存储图表数量
    QGridLayout *m_gridLayout; // 存储网格布局指针，方便重建布局
    QScrollArea *m_scrollArea;
    QWidget *m_scrollAreaWidgetContents;
};

