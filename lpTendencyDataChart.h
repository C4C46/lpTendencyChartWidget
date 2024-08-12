#pragma once

#include <QObject>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_scale_div.h>
#include <QVBoxLayout>
#include <QTime>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDoubleValidator>
#include <QStringList>
#include <QSplitter> 
#include <qwt_plot_magnifier.h>
#include <qwt_text.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <qwt_scale_draw.h>
#include <QMouseEvent>
#include <QHash>
#include <QButtonGroup>
#include <QMessageBox>
#include <QRadioButton>
#include <QTimer>
#include <QScrollArea>
#include "lpTendencyChartConfig.h"
#include "lpTendencyChartUpdateThread.h"

// 自定义的 ScaleDraw 类，用于格式化标签
class CustomScaleDraw : public QwtScaleDraw {
public:
	CustomScaleDraw() {}

	virtual QwtText label(double value) const override {
		// 使用 'f' 格式化，无小数点，	且不使用科学计数法
		return QwtText(QString::number(value, 'f', 0));
	}
};

class lpTendencyDataChart : public QObject
{
	Q_OBJECT

public:
	lpTendencyDataChart(QObject *parent, QWidget *parentWidget, const QStringList &curveNames,
		lpTendencyChartConfig* configLoader);
	~lpTendencyDataChart();
	double adjustXValue(double originalX);
protected:
	bool eventFilter(QObject *watched, QEvent *event) override; //界面操作规则

public:
	QString determineParentCategory(const QString & alignmentName);//确定父类别（陶瓷、极耳、电浆）
	bool isSameCategory(const QString & option1, const QString & option2);//判断是否是统一类别，进行对齐度比对
	void resetCurvesOpacity(); // 添加成员变量存储曲线名称
	void installEventFilters();//恢复所有曲线显示
    QwtPlot* getPlot() const { return m_plot; }  // 添加获取图表的方法
    QSlider* getSlider() { return m_slider; }
public slots:
	void onChartUpdate(const QString &curveName, double x, double y);
	void onIntervalPBClicked();//参数设置
	void AlignPBClicked();//对齐度设置
	void addCurve(const QString &curveName); // 添加曲线
	void onLegendClicked(const QVariant &itemInfo, int index);//点击对应曲线名称，曲线进行凸显
	void onCurveDisplayChanged(const QString &curveName, bool display); // 曲线状态改变
	void updateYAxisRange(const QVariantList & yAxisRange);//更新y轴范围
	void updateWarningValue(const QVariantList & warningValue);//更新预警值范围
	void updateAlarmValue(const QVariantList & alarmValue);//更新报警值范围
	void onSliderValueChanged(int value);//趋势图拖拽进度条控制
	void updateSliderPosition();//更新进度条滑块的位置
	void batchUpdateChart(); // 批量更新图表
	void clearChart();//清空趋势图曲线内容

private:
	QWidget *m_widget;
	QwtPlot *m_plot;
	lpTendencyChartConfig *m_configLoader{ nullptr };
	QVector<QwtPlotCurve *> m_curves; // 支持多条曲线
	QMap<QString, QVector<double>> m_xDataMap, m_yDataMap; // 存储每条曲线的数据
	QStringList m_curveNames;

	double xInterval = 10; // 默认x间隔值
	//double yInterval = 10; // 默认y间隔值
	QTableWidget *m_table; // 添加表格成员变量
	bool m_autoScrollEnabled = true; // 默认启用自动滚动
	//预警和报警阈值范围
	double m_warningValueLower;
	double m_warningValueUpper;
	double m_alarmValueLower;
	double m_alarmValueUpper;
	QPoint m_dragStartPosition; // 用于跟踪拖动开始时的鼠标位置
	double m_xMinCurrent, m_xMaxCurrent; // 用于跟踪当前图表的X轴范围
	bool m_isDragging = false; // 标记是否正在拖动
	bool m_isViewingHistory = false;
	QSlider *m_slider;//趋势图滚动条
	int m_replotCount = 0;
	QTimer *m_updateTimer; // 用于批量更新图表
	bool m_hasNewData = false;
	double m_lastResetX = 0; // 用于记录上次重置的x值
	double m_maxReceivedX = 0; //存储接收到的最大X值
};

