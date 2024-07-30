

//#include "lptendencychartwidget_global.h"

#pragma once

#include <QtCore/qglobal.h>
#include <QtWidgets>
#include <QMap>
#include <QVariantMap>
#include "lp_logger.h"

#ifndef BUILD_STATIC
# if defined(LPTENDENCYCHARTWIDGET_LIB)
#  define LPTENDENCYCHARTWIDGET_EXPORT Q_DECL_EXPORT
# else
#  define LPTENDENCYCHARTWIDGET_EXPORT Q_DECL_IMPORT
# endif
#else
# define LPTENDENCYCHARTWIDGET_EXPORT
#endif


struct LithiumChannelRegionInfo_Tag
{
	int msgId = -1; // 消息id
	qint64 posMeterMm{ -1 };
	QMap<int, QVariantMap> channelIdInfoMp; // <通道，<xxx宽度，宽度值>>
	QMap<int, QVariantMap> channelIdAlignmentInfoMp; // <通道，<xxx对齐度，对齐度值>>
	QVariantMap meterAllDetectTypeAndDetailsInfo;
	QVariantMap channelAllDetectTypeExtendInfo; // <米数，<名称，宽度>>
};


class lpTendencyChartWidgetPrivate;


class LPTENDENCYCHARTWIDGET_EXPORT lpTendencyChartWidget : public QWidget 
{
	Q_OBJECT
public:
	lpTendencyChartWidget(QWidget *parent);
	~lpTendencyChartWidget();

	//锂电
	virtual void initLithiumChart(QWidget *pWidget = nullptr);
	virtual void initLithiumScope(QWidget *pWidget = nullptr);
	void updateLithiumChart(LithiumChannelRegionInfo_Tag *tag);

	//通用
	void DataScope(const QString &curveName, double x, double y);//接收数据更新数据表格
	void DataChart(const QString &curveName, double x, double y);//接收数据更新趋势图

public slots:
	void clearLithiumData();
	void onSaveLithiumRecipe(QString fileName, QString obj);
private:
	QSharedPointer<lpTendencyChartWidgetPrivate> d_ptr;//管理趋势图表格
};

//class LPTENDENCYCHARTWIDGET_EXPORT lpTendencyChartWidget : public QWidget 
//{
//	Q_OBJECT
//public:
//	lpTendencyChartWidget(QWidget *parent);
//	~lpTendencyChartWidget();
//
//	//锂电
//	virtual void initLithiumChart(QWidget *pWidget = nullptr);
//	virtual void initLithiumScope(QWidget *pWidget = nullptr);
//	void updateLithiumChart(LithiumChannelRegionInfo_Tag *tag);
//
//	//通用
//	void DataScope(const QString &curveName, double x, double y);//接收数据更新数据表格
//	void DataChart(const QString &curveName, double x, double y);//接收数据更新趋势图
//
//private:
//	QSharedPointer<lpTendencyChartWidgetPrivate> d_ptr;//管理趋势图表格
//};