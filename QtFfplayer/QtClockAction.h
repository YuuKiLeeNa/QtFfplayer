#pragma once
#include<QWidgetAction>
#include<vector>

class QButtonGroup;
class QRadioButton;

class QtClockAction :public QWidgetAction
{
	Q_OBJECT
public:
	QtClockAction(QWidget* pParentWidget);
	~QtClockAction();
signals:
	void stateChanged(int);
protected:
	void initUI();

protected:
	QWidget* m_wid;
	QButtonGroup* m_btnGroup = nullptr;
	QRadioButton* m_radioExt = nullptr;
	QRadioButton* m_radioAudio = nullptr;
	QRadioButton* m_radioVideo = nullptr;

	std::vector<std::tuple<QRadioButton* QtClockAction::*, QString,int>>m_vAudioInfo =
	{
		{&QtClockAction::m_radioAudio,tr("audio"),0}
		,{&QtClockAction::m_radioVideo,tr("video"),1}
		,{&QtClockAction::m_radioExt,tr("ext"),2}
	};

};

