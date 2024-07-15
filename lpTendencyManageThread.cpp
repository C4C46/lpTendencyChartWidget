#include "lpTendencyManageThread.h"

lpTendencyManageThread::lpTendencyManageThread()
{

}


lpTendencyManageThread::~lpTendencyManageThread()
{

}

void lpTendencyManageThread::stopThread()
{
	QMutexLocker locker(&m_mutex);
}

void lpTendencyManageThread::threadStart()
{
	qDebug() << "lpTendencyManageThreadID: " << QThread::currentThreadId();
	if (!m_currentTag) return;

	qint64 x = static_cast<double>(m_currentTag->posMeterMm);

	// 遍历 channelIdInfoMp
	for (auto it = m_currentTag->channelIdInfoMp.begin(); it != m_currentTag->channelIdInfoMp.end(); ++it) {
		int channelId = it.key();
		QVariantMap info = it.value();
		for (auto detailIt = info.begin(); detailIt != info.end(); ++detailIt) {
			QString name = detailIt.key(); // 获取名称
			double width = detailIt.value().toDouble(); // 获取宽度值
			emit sgupdateScope(name, x, width);//更新表格
			emit sgupdateChart(name, x, width);//更新趋势图
		}
	}

	// 遍历 channelIdAlignmentInfoMp
	for (auto it = m_currentTag->channelIdAlignmentInfoMp.begin(); it != m_currentTag->channelIdAlignmentInfoMp.end(); ++it) {
		int channelId = it.key();
		QVariantMap alignmentInfo = it.value();
		for (auto detailIt = alignmentInfo.begin(); detailIt != alignmentInfo.end(); ++detailIt) {
			QString name = detailIt.key(); // 获取对齐度名称
			double alignmentValue = detailIt.value().toDouble(); // 获取对齐度值
			emit sgupdateScope(name, x, alignmentValue);//更新表格
			emit sgupdateChart(name, x, alignmentValue);//更新趋势图
		}
	}

	// 解析 channelAllDetectTypeExtendInfo
	QVariantMap extendInfo = m_currentTag->channelAllDetectTypeExtendInfo;
	for (auto it = extendInfo.begin(); it != extendInfo.end(); ++it) {
		QString name = it.key(); // 获取检测类型名称
		double value = it.value().toDouble(); // 获取对应的值
		emit sgupdateScope(name, x, value);//更新表格
		emit sgupdateChart(name, x, value);//更新趋势图
	}

}







void lpTendencyManageThread::onUpdateDataScope(LithiumChannelRegionInfo_Tag * tag)
{
	QMutexLocker locker(&m_mutex);
	m_currentTag = tag;
	threadStart();


}