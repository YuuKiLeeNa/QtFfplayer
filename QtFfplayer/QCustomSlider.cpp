#include "QCustomSlider.h"
#include<QMouseEvent>
#include<functional>
#include<algorithm>
#include<numeric>
#include<QDebug>

QCustomSlider::QCustomSlider(const std::function<void(double, double)>& funDragCallBack, QWidget* parentWidget) :QSlider(parentWidget), m_dragCallBack(std::make_unique<decltype(m_dragCallBack)::element_type> (funDragCallBack))
{
	setOrientation(Qt::Orientation::Horizontal);
	setMouseTracking(true);
}

void QCustomSlider::setValueAccordMousePos(const QPoint& pt) 
{
	int x = pt.x();
	int sliderWidth = width();
	double v = (double)x / sliderWidth;
	int vMax = maximum();
	int vMin = minimum();
	m_iCurRealValue = vMin + v * (vMax - vMin);
	int iSrcV = value() / 1000000;
	QSlider::setValue(m_iCurRealValue);
	if (m_dragCallBack)
	{
		double dv = m_iCurRealValue / 1000000;

		double offV = dv - iSrcV;
		if (offV > 0)
			offV = std::max(10.0, dv - iSrcV);
		else if (offV < 0)
			offV = std::min(-10.0, dv - iSrcV);

		(*m_dragCallBack)(dv, offV);
	}
}

void QCustomSlider::setValue(int v) 
{
	qDebug() <<"receive time"<< v;
	m_iCurRealValue = v;
	if (!m_bMousePressed)
		QSlider::setValue(m_iCurRealValue);
}

void QCustomSlider::setDragValueCallBack(const std::function<void(double, double)>& fun)
{
	m_dragCallBack = std::make_unique<std::function<void(double, double)>>(fun);
}

void QCustomSlider::mouseMoveEvent(QMouseEvent* ev) 
{
	QPoint pt = ev->pos();
	if (m_bMousePressed && m_iSaveXValue != pt.x())
		setValueAccordMousePos(pt);
}

void QCustomSlider::mousePressEvent(QMouseEvent* ev) 
{
	m_bMousePressed = true;
	QPoint ptr = ev->pos();
	setValueAccordMousePos(ev->pos());
	m_iSaveXValue = ptr.x();
}

void QCustomSlider::mouseReleaseEvent(QMouseEvent* ev) 
{
	m_bMousePressed = false;
	QSlider::setValue(m_iCurRealValue);
}