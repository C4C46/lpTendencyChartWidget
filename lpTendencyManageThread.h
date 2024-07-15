#pragma once
#include <qobject.h>
#include <QMap>
#include <QVariantList>
#include <QMutexLocker>
#include <QMutex>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include "lpTendencyChartWidget.h"
//struct LithiumChannelRegionInfo_Tag
//{
//	int msgId = -1; // 消息id
//	qint64 posMeterMm{ -1 };
//	QMap<int, QVariantMap> channelIdInfoMp; // <通道，<xxx宽度，宽度值>>
//	QMap<int, QVariantMap> channelIdAlignmentInfoMp; // <通道，<xxx对齐度，对齐度值>>
//	QVariantMap meterAllDetectTypeAndDetailsInfo;
//	QVariantMap channelAllDetectTypeExtendInfo; // <米数，<名称，宽度>>
//};

class lpTendencyManageThread : public QObject
{
	Q_OBJECT

public:
	lpTendencyManageThread();
	~lpTendencyManageThread();

	void stopThread();
	void threadStart();

signals:
	void sgupdateScope(const QString &curveName, double x, double y);
	void sgupdateChart(const QString &curveName, double x, double y);

public slots:
	void onUpdateDataScope(LithiumChannelRegionInfo_Tag * tag);

private:
	mutable QMutex m_mutex;
	LithiumChannelRegionInfo_Tag *m_currentTag;

};

