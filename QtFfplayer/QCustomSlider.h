#pragma once
#include <qslider.h>
#include<atomic>
#include<functional>
#include<memory>
class QMouseEvent;

class QCustomSlider :public QSlider
{
	Q_OBJECT
public:
	QCustomSlider(const std::function<void(double, double)>&funDragCallBack,QWidget*parentWidget = nullptr);
	void setValueAccordMousePos(const QPoint&pt);
	//bool getMouseState();
	void setValue(int v);

	void setDragValueCallBack(const std::function<void(double, double)>&fun);


protected:
	virtual void mouseMoveEvent(QMouseEvent* ev) override;
	virtual void mousePressEvent(QMouseEvent* ev) override;
	virtual void mouseReleaseEvent(QMouseEvent* ev) override;
protected:
	std::atomic_bool m_bMousePressed = false;
	int m_iCurRealValue = 0;
	std::unique_ptr<std::function<void(double,double)>>m_dragCallBack;
	int m_iSaveXValue;
};
