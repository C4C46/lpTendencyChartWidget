#include "lpTendencyChartUpdateThread.h"


lpTendencyChartUpdateThread::lpTendencyChartUpdateThread()
{



}



lpTendencyChartUpdateThread::~lpTendencyChartUpdateThread()
{

	m_dataCache.clear();
}

void lpTendencyChartUpdateThread::threadStart()
{
	qDebug() << "threadStartThreadID:" << QThread::currentThreadId();
	for (auto &curveName : m_dataCache.keys()) {
		for (auto &data : m_dataCache[curveName]) {
			double x = data.first;
			double y = data.second.first;
			QVariantList warningValue = data.second.second[0].toList();
			QVariantList alarmValue = data.second.second[1].toList();
			emit sgSendData(curveName, x, y, warningValue, alarmValue);

		}

	}
	m_dataCache.clear(); // 清空缓存
	m_processFlag = false;
}







void lpTendencyChartUpdateThread::onDataCache(QMap<QString, QList<QPair<double, QPair<double, QVariantList>>>> dataCache)
{
	//qDebug() << "onDataCache called with dataCache size:" << dataCache.size();

	if (m_processFlag == true)
	{
		return;
	}
	m_dataCache = dataCache;
	threadStart();

}
