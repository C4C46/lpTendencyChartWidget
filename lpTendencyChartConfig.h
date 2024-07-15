#pragma once

#include <QObject>
#include <QTableWidget>
#include <QTreeWidget>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>
#include <QButtonGroup>
#include <QRadioButton>
#include <QMessageBox>

class lpTendencyChartConfig :public QObject
{
	Q_OBJECT

public:
	explicit lpTendencyChartConfig(QTreeWidget *treeWidget, QObject *parent = nullptr);
	QStringList getAllCurveNamesExceptParent() const;//获取除 包含“对齐度”内容的其他子类名称
	QVariantMap getSettingDefaultValue(const QString & settingName);//获取对应父类名称下的属性内容（y的范围，预警值范围，报警值范围）
	void loadConfig(const QString &filePath);//读取配置文件内容
	void saveConfig(const QString &filePath); // 添加保存配置的声明
	QString getSelectedParentNames() const;//获取选择的父类名称
	void updateSetting(const QString & settingName, const QString & key, const QVariantList & value);//参数设置实时修改
	void addNewChildToCategory(const QString & categoryName, const QString & childName, bool display);//添加新的对齐度匹配名称
	void removeChildFromCategory(const QString & categoryName, const QString & childName);	//删除对齐度名称
	QStringList getCurveNames() const; // 选中的子类名称
	QString getParentNameForCurve(const QString & curveName) const;//子类所属父类
	QStringList getParentCategoryNames() const;//添加到对应父类名称中
	QStringList getAllCurveNames() const;//获取所有子类名称

signals:
	void curveDisplayChanged(const QString &curveName, bool display);//发送勾选子类名称以及对应复选框状态
	void yAxisRangeChanged(const QVariantList &yAxisRange);//发送y轴的范围
	void warningValueChanged(const QVariantList &warningValue);//发送预警值范围
	void alarmValueChanged(const QVariantList &alarmValue);//发送报警值范围
	void curveNamesChanged(const QStringList &newCurveNames) const;//发送每次所选子类名称

private:
	QTreeWidget *m_treeWidget;
	QStringList m_curveNames; // 存储曲线名称
	QButtonGroup *m_buttonGroup; // 管理单选按钮的按钮组
	QJsonDocument m_configDoc; // 添加报错配置文件信息成员变量

};

