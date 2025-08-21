#include "QProgressSlider.h"
#include<QMouseEvent>
#include<functional>
#include<algorithm>
#include<numeric>
#include<QDebug>
#include<QLabel>
#include<qboxlayout.h>

#define HANDLE_WIDTH 16
#define HANDLE_HEIGHT 16

#define MOUSE_MOVE_HANDLE(X) m_Pos->move(X-HANDLE_WIDTH/2, (height() - HANDLE_HEIGHT)/2)

QProgressSlider::QProgressSlider(const std::function<void(long long, long long)>& funDragCallBack, QWidget* parentWidget) :QWidget(parentWidget), m_dragCallBack(std::make_unique<decltype(m_dragCallBack)::element_type> (funDragCallBack))
{
	setFixedHeight(HANDLE_HEIGHT);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	initUI();
	setMouseTracking(true);
	setRange(0,100000000);
	setEnabled(false);
	setValueAccordMousePos(0);
}

void QProgressSlider::setValueAccordMousePos(int x)
{
	int sliderWidth = width();
	double dMousePos = x - HANDLE_WIDTH / 2;
	if (dMousePos < 0)
		dMousePos = 0;
	else if (dMousePos > sliderWidth - HANDLE_WIDTH)
		dMousePos = sliderWidth - HANDLE_WIDTH;

	double v = dMousePos / (sliderWidth - HANDLE_WIDTH);
	long long vMax = m_llMaxV;
	long long vMin = m_llMinV;

	long long llSrcV = m_llCurRealValue;
	m_llCurRealValue = vMin + v * (vMax - vMin);

	if (m_dragCallBack)
	{
		long long lloffV = m_llCurRealValue - llSrcV;
		if (lloffV > 0)
			lloffV = std::max((long long)10000000, m_llCurRealValue - llSrcV);
		else if (lloffV < 0)
			lloffV = std::min((long long)-10000000, m_llCurRealValue - llSrcV);

		(*m_dragCallBack)(m_llCurRealValue, lloffV);
	}
}

void QProgressSlider::setValue(long long v, bool bConsideMousePress, bool bIsChangeHandle)
{
	if (v < m_llMinV)
		v = m_llMinV;
	else if (v > m_llMaxV)
		v = m_llMaxV;

	//qDebug() <<"receive time"<< v;
	m_llCurRealValue = v;
	if (bIsChangeHandle && ((bConsideMousePress && !m_bMousePressed) || !bConsideMousePress))
	{
		long double factor = long double(m_llCurRealValue - m_llMinV) / (m_llMaxV - m_llMinV);
		m_Pos->move(factor* (width() - HANDLE_WIDTH), (height() - HANDLE_HEIGHT) / 2);
	}
}

void QProgressSlider::setDragValueCallBack(const std::function<void(long long, long long)>& fun)
{
	m_dragCallBack = std::make_unique<std::function<void(long long, long long)>>(fun);
}

void QProgressSlider::setRange(long long llMin, long long llMax)
{
	m_llMinV = llMin;
	m_llMaxV = llMax;
	if (m_llMinV > m_llMaxV)
		setEnabled(false);
	else
		setEnabled(true);
}
long long QProgressSlider::getValueAccordBarPos()
{
	QPoint pt = m_Pos->pos();
	long long x = pt.x();
	long long ibarLen = width() - m_Pos->width();
	return (double)x / ibarLen * (m_llMaxV - m_llMinV);
}
//void QProgressSlider::setHandleOffSetPos(long long v)
//{
//	/*int destV = v + m_llV;
//	if (destV < m_llMinV)
//		destV = m_llMinV;
//	else if (destV > m_llMaxV)
//		destV = m_llMaxV;*/
//	int64_t tmpV = v + m_llV;
//	//setValue(tmpV);
//	(*m_dragCallBack)(tmpV, v);
//
//}

void QProgressSlider::initUI() 
{
	QWidget* grov = new QWidget(this);
	grov->setFixedHeight(6);
	grov->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	grov->setObjectName("grov");
	grov->setStyleSheet("QWidget#grov{background-color:#30FFFFFF;border-radius:3px;}");
	QVBoxLayout* mainL = new QVBoxLayout;
	mainL->setContentsMargins(0,0,0,0);
	mainL->setSpacing(0);
	mainL->setAlignment(Qt::AlignVCenter);
	mainL->addWidget(grov);
	setLayout(mainL);

	m_Pos = new QLabel(this);
	m_Pos->setObjectName("m_widPos");
	m_Pos->setFixedSize(HANDLE_WIDTH, HANDLE_HEIGHT);
	m_Pos->setStyleSheet(R"(QLabel#m_widPos{border-radius:8px;background-color:#A0FFFFFF;}
QLabel#m_widPos:hover{border-radius:8px;background-color:#1296DB;})");
	MOUSE_MOVE_HANDLE(HANDLE_WIDTH /2);
}

void QProgressSlider::mouseMoveEvent(QMouseEvent* ev)
{
	int xV = ev->x();
	xV = getXMouseInRange(xV);
	if (m_bMousePressed && m_iSaveXValue != xV)
	{
		MOUSE_MOVE_HANDLE(xV);
		setValueAccordMousePos(xV);
	}
}

void QProgressSlider::mousePressEvent(QMouseEvent* ev)
{
	m_bMousePressed = true;
	int ptrX = ev->x();
	ptrX = getXMouseInRange(ptrX);
	MOUSE_MOVE_HANDLE(ptrX);
	setValueAccordMousePos(ptrX);
	m_iSaveXValue = ptrX;
}

void QProgressSlider::mouseReleaseEvent(QMouseEvent* ev)
{
	m_bMousePressed = false;
	setValue(m_llCurRealValue,false, false);
}

void QProgressSlider::resizeEvent(QResizeEvent* event) 
{
	setValue(m_llCurRealValue,false);
}

int QProgressSlider::getXMouseInRange(int xCoord) const
{
	int widWidth = width();
	if (xCoord < HANDLE_WIDTH/2)
		xCoord = HANDLE_WIDTH / 2;
	else if (xCoord > widWidth - HANDLE_WIDTH/2)
		xCoord = widWidth - HANDLE_WIDTH / 2;
	return xCoord;
}

