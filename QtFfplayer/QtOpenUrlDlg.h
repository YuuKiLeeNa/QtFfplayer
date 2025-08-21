#pragma once
#include <qdialog.h>
#include<QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QPushButton;
class QCheckBox;

class QtOpenUrlDlg:public QDialog
{
	Q_OBJECT
public:
	QtOpenUrlDlg(QWidget*parentWidget = nullptr);
	QString getUrl();
	//std::pair<bool,QString> getSaveFilePath();
protected slots:
	void slotOkBtnClicked();
	void slotCancelBtnClicked();
	//void slotBoxChecked(bool b);
protected:
	QLineEdit* m_editUrl;
	QPushButton* m_okBtn;
	QPushButton* m_cancelBtn;

	//QLineEdit* m_editSavePath;
	//QCheckBox* m_boxSave;
	//QLabel* m_labelSave;
};

