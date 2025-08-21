#include "QtClockAction.h"
#include<QRadioButton>
#include<qbuttongroup.h>
#include<QBoxLayout>

QtClockAction::QtClockAction(QWidget* pParentWidget) :QWidgetAction(pParentWidget)
{
	initUI();
	QObject::connect(m_btnGroup, SIGNAL(buttonPressed(int)), this,SIGNAL(stateChanged(int)));
}

QtClockAction::~QtClockAction()
{
	delete m_wid;
}

void QtClockAction::initUI() 
{
	m_wid = new QWidget();
	m_wid->setObjectName("m_wid");
	m_wid->setStyleSheet(R"(
QWidget#m_wid
{
	background-color:transparent;
}
QRadioButton
{
	color:white;
}

QRadioButton::indicator {
    width: 13px;
    height: 13px;
}

QRadioButton::indicator::unchecked {
    image: none;
}

QRadioButton::indicator:checked {
    image: url(:/icon/res/radioHover.png);
}
)");
	QVBoxLayout* widL = new QVBoxLayout;
	widL->setContentsMargins(0, 0, 0, 0);
	m_btnGroup = new QButtonGroup(m_wid);
	m_btnGroup->setExclusive(true);
	for (auto& ele : m_vAudioInfo)
	{
		this->*std::get<0>(ele) = new QRadioButton(std::get<1>(ele), m_wid);
		widL->addWidget(this->*std::get<0>(ele));
		m_btnGroup->addButton(this->*std::get<0>(ele), std::get<2>(ele));
	}
	m_radioAudio->setChecked(true);
	m_wid->setLayout(widL);
	setDefaultWidget(m_wid);
}

