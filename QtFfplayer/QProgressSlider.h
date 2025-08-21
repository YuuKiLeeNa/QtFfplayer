#pragma once
#include <qslider.h>
#include<atomic>
#include<functional>
#include<memory>
#include<QWidget>
#include<qframe.h>
class QMouseEvent;
class QLabel;
class QProgressSlider :public QWidget
{
	Q_OBJECT
public:
	QProgressSlider(const std::function<void(long long, long long)>&funDragCallBack,QWidget*parentWidget = nullptr);
	void setValueAccordMousePos(int x);
	void setValue(long long v, bool bConsideMousePress = true,bool bIsChangeHandle = true);
	void setDragValueCallBack(const std::function<void(long long, long long)>&fun);
	void setRange(long long llMin, long long llMax);
	
	long long minimum() const 
	{
		return m_llMinV;
	}
	long long maximum() const 
	{
		return m_llMaxV;
	}
	long long value() const 
	{
		return m_llCurRealValue;
	}
	long long getValueAccordBarPos();
	//void setHandleOffSetPos(long long v);
protected:
	void initUI();
	virtual void mouseMoveEvent(QMouseEvent* ev) override;
	virtual void mousePressEvent(QMouseEvent* ev) override;
	virtual void mouseReleaseEvent(QMouseEvent* ev) override;
	virtual void resizeEvent(QResizeEvent* event)override;
	int getXMouseInRange(int xCoord) const;
	//void setValueWithOutChangeHandle(long long v);
protected:
	std::atomic_bool m_bMousePressed = false;
	long long m_llCurRealValue = 0;
	//long long m_llV = 0;
	long long m_llMinV = 0;
	long long m_llMaxV = 99999999;
	std::unique_ptr<std::function<void(long long, long long)>>m_dragCallBack;
	int m_iSaveXValue;
	QLabel* m_Pos;
};
