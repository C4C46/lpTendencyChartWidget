#include "lpTendencyChartWidget.h"
#include "lpTendencyChartWidgetPrivate.h"
lpTendencyChartWidget::lpTendencyChartWidget(QWidget* parent):
	QWidget(parent), d_ptr(new lpTendencyChartWidgetPrivate(this))
{
	lp_log::instance()->set_record_level(LP_DEBUG);
	lp_log::instance()->enable_qt_message_handler(true, "Lithium");

}


lpTendencyChartWidget::~lpTendencyChartWidget()
{

}

void lpTendencyChartWidget::initLithiumChart(QWidget * pWidget)
{
	if (!pWidget) {
		pWidget = new QWidget(this); // 如果没有提供widget，则创建一个新的
	}

	QVBoxLayout *layout = new QVBoxLayout(pWidget); // 创建布局
	layout->addWidget(d_ptr->getTopWidget()); // 将 top_widget 添加到布局中
	pWidget->setLayout(layout); // 设置widget的布局
}

void lpTendencyChartWidget::initLithiumScope(QWidget * pWidget)
{

	if (!pWidget) {
		pWidget = new QWidget(this); // 如果没有提供widget，则创建一个新的
	}

	QVBoxLayout *layout = new QVBoxLayout(pWidget); // 创建布局
	layout->addWidget(d_ptr->getDownWidget()); // 将 down_widget 添加到布局中
	pWidget->setLayout(layout); // 设置widget的布局
}

void lpTendencyChartWidget::updateLithiumChart(LithiumChannelRegionInfo_Tag * tag)
{

	qDebug() << "updateLithiumChartThreadID: " << QThread::currentThreadId();

	d_ptr->emit sgUpdateDataLithium(tag);
	//if (!tag) return;

	//double x = static_cast<double>(tag->posMeterMm);

	//// 遍历 channelIdInfoMp
	//for (auto it = tag->channelIdInfoMp.begin(); it != tag->channelIdInfoMp.end(); ++it) {
	//	int channelId = it.key();
	//	QVariantMap info = it.value();
	//	for (auto detailIt = info.begin(); detailIt != info.end(); ++detailIt) {
	//		QString name = detailIt.key(); // 获取名称
	//		double width = detailIt.value().toDouble(); // 获取宽度值
	//		d_ptr->emit sgUpdateDataScope(name, x, width);//更新表格
	//		d_ptr->emit sgUpdateDataChart(name, x, width);//更新趋势图
	//	}
	//}

	//// 遍历 channelIdAlignmentInfoMp
	//for (auto it = tag->channelIdAlignmentInfoMp.begin(); it != tag->channelIdAlignmentInfoMp.end(); ++it) {
	//	int channelId = it.key();
	//	QVariantMap alignmentInfo = it.value();
	//	for (auto detailIt = alignmentInfo.begin(); detailIt != alignmentInfo.end(); ++detailIt) {
	//		QString name = detailIt.key(); // 获取对齐度名称
	//		double alignmentValue = detailIt.value().toDouble(); // 获取对齐度值
	//		d_ptr->emit sgUpdateDataScope(name, x, alignmentValue);//更新表格
	//		d_ptr->emit sgUpdateDataChart(name, x, alignmentValue);//更新趋势图
	//	}
	//}

	//// 解析 channelAllDetectTypeExtendInfo
	//QVariantMap extendInfo = tag->channelAllDetectTypeExtendInfo;
	//for (auto it = extendInfo.begin(); it != extendInfo.end(); ++it) {
	//	QString name = it.key(); // 获取检测类型名称
	//	double value = it.value().toDouble(); // 获取对应的值
	//	d_ptr->emit sgUpdateDataScope(name, x, value);//更新表格
	//	d_ptr->emit sgUpdateDataChart(name, x, value);//更新趋势图
	//}

}

void lpTendencyChartWidget::DataScope(const QString & curveName, double x, double y)
{
	d_ptr->emit sgUpdateData(curveName, x, y);//更新表格
}

void lpTendencyChartWidget::DataChart(const QString & curveName, double x, double y)
{
	d_ptr->emit sgUpdateData(curveName, x, y);//更新趋势图

}

