#include "QtOpenUrlDlg.h"
#include<QLabel>
#include<QLineEdit>
#include<qboxlayout.h>
#include<QPushButton>
#include<QCheckBox>
#include<QCoreApplication>
#include<QDateTime>
#define WIDTH_WID 190

QtOpenUrlDlg::QtOpenUrlDlg(QWidget* parentWidget):QDialog(parentWidget)
{
	setWindowTitle(tr("open Url"));
	this->setStyleSheet(R"(QPushButton{border:1px solid gray;color:black;}
QPushButton:hover{border:1px solid gray;color:black;background-color:#ADD8E6;}
QPushButton:pressed{border:1px solid gray;color:black;background-color:#4F94CD;}
QCheckBox{background-color:transparent;}
QLabel{color:black;}
)");

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setContentsMargins(0,0,0,0);

	QLabel* label = new QLabel(tr("Url"), this);

	label->setScaledContents(true);

	QHBoxLayout* labelL = new QHBoxLayout;
	labelL->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	labelL->setContentsMargins(0,0,0,0);
	labelL->addWidget(label);
	labelL->addStretch();

	m_editUrl = new QLineEdit(this);
	m_editUrl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);


	//m_boxSave = new QCheckBox(tr("save"),this);
	//m_boxSave->setChecked(false);


	m_okBtn = new QPushButton(tr("ok"), this);
	m_cancelBtn = new QPushButton(tr("cancel"), this);
	const int w = 75,h = 20;
	m_okBtn->setFixedSize(w,h);
	m_cancelBtn->setFixedSize(w,h);
	QHBoxLayout* btnL = new QHBoxLayout;
	btnL->setContentsMargins(0,0,15,0);
	btnL->setAlignment(Qt::AlignVCenter);
	btnL->addStretch();
	btnL->addWidget(m_okBtn);
	btnL->addWidget(m_cancelBtn);


	QHBoxLayout* editL = new QHBoxLayout;
	editL->setContentsMargins(0, 0, 0, 0);
	editL->addWidget(m_editUrl);
	//editL->addWidget(m_boxSave);
	editL->addStretch();


	//m_labelSave = new QLabel(tr("save path:"),this);
	//m_labelSave->setScaledContents(true);
	//m_editSavePath = new QLineEdit(this);
	//m_editSavePath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	//QHBoxLayout* saveLay = new QHBoxLayout;
	//saveLay->setContentsMargins(0, 0, 0, 0);
	//saveLay->addWidget(m_labelSave);
	//saveLay->addWidget(m_editSavePath);
	//saveLay->addStretch();

	QVBoxLayout* widL = new QVBoxLayout;
	widL->setContentsMargins(15,0,15,0);
	widL->setAlignment(Qt::AlignCenter);

	widL->addLayout(labelL);
	widL->addLayout(editL);
	//widL->addLayout(saveLay);
	widL->addLayout(btnL);
	
	setLayout(widL);


	m_okBtn->setFocus();
	QObject::connect(m_okBtn, SIGNAL(clicked()),this, SLOT(slotOkBtnClicked()));
	QObject::connect(m_cancelBtn, SIGNAL(clicked()), this, SLOT(slotOkBtnClicked()));
	//QObject::connect(m_boxSave, SIGNAL(toggled(bool)), this, SLOT(slotBoxChecked(bool)));
}

QString QtOpenUrlDlg::getUrl() 
{
	return m_editUrl->text();
}

//std::pair<bool, QString> QtOpenUrlDlg::getSaveFilePath()
//{
//	//return { m_boxSave->isChecked(), m_editSavePath->text() };
//}

void QtOpenUrlDlg::slotOkBtnClicked() 
{
	return accept();
}

void QtOpenUrlDlg::slotCancelBtnClicked() 
{
	return reject();
}

//void QtOpenUrlDlg::slotBoxChecked(bool b) 
//{
//	if (!b)
//		m_editSavePath->setText("");
//	else 
//	{
//		QString savePath = QCoreApplication::applicationDirPath() + "/save/"+ QDateTime::currentDateTime().toString("yyyyMMdd hhmmss")+".ts";
//		m_editSavePath->setText(savePath);
//	}
//}