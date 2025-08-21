#pragma once
#include<QTableView>
#include<vector>
#include<memory>
#include<functional>
#include<set>
#include<QAction>

class QProgressSlider;
class QtNoFocusBorderItemDelegate;
class QtPlayListModel;
class QtFfplayer;
class QLabel;
class QMenu;

class QtPlayList :public QTableView
{
Q_OBJECT
public:
	enum PLAY_MODE { PLAY_MODEL_SINGLE, PLAY_MODEL_SINGLE_CIRCLE, PLAY_MODEL_LIST_ORDER, PLAY_MODEL_LIST_CIRCEL, PLAY_MODEL_LIST_RAND};
	QtPlayList(QtFfplayer* pQMediaPlayer,QProgressSlider*slider, QWidget*parentWidget = nullptr);
	std::pair<QString, int> insertList(int index,QStringList&& playList);
	std::vector<std::pair<QString, QString>>getFilePathList();
	std::pair<QString,int> appendList(QStringList&& playList);
	QString setPlayIndex(int index);
	std::pair<QString,int> NextPlayFileName();
	//void UpdateProgress(int iIndexOfRows);
	void setPlayModelCallBack(const std::function<PLAY_MODE()>&funPlayModel);
	std::pair<QString, int> getCurPlayIndexFile()const;
	static QString getFileName(const QString& strFilePath);
	QtPlayListModel* getModel()const
	{
		return m_model;
	}
	void PlayingRow(const QModelIndex& index);
	inline std::set<int> getSelectedRowSets()const 
	{
		return m_vSelectedRow;
	}
	void setNeedToMakeRandList(bool b = true) 
	{
		m_bIsNeedToMakeRandList = b;
	}
	std::vector<QString>getAllPathFilesInList();
	void setControlKeyPressed(bool b = true);
public slots:
	void slotSetSelected(int row);
	void slotProgressBarValueChange();
	void slotRowDoubleClicked(const QModelIndex &index);
	void slotTimeOut();
	//void slotRightMouseButtonClicked();
	//void slotVerticalScrollBar(int v);
	//void slotUpdateProgress();
protected:
	void mousePressEvent(QMouseEvent* event)override;
	void mouseMoveEvent(QMouseEvent* event)override;
	void mouseReleaseEvent(QMouseEvent* event)override;
	//void leaveEvent(QEvent* event)override;
	//void enterEvent(QEvent* event) override;
	//void updateHoverRow(int row);
	void makeRandList();
	void initConnect();

	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event)override;
	void dropEvent(QDropEvent* event) override;
	void keyPressEvent(QKeyEvent* event)override;
	void keyReleaseEvent(QKeyEvent* event)override;
	void focusOutEvent(QFocusEvent* event)override;
	//void mousePressEvent(QMouseEvent* event) override;
	void DoDrag();
	void MoveRow(int from, int to);//,QStringList&&list);
	void updateRandPlayList(const std::map<int,int>&oriToNew);
protected:
	void contextMenuEvent(QContextMenuEvent* event)override;
	void slotRemoveActionClicked();
	void slotInsertActionClicked();
	void slotAddActionClicked();

	std::vector<std::tuple<QString, QAction*,void (QAction::*)(bool), void(QtPlayList::*)()>>m_initMenuInfo =
	{
		{tr("remove"), nullptr, &QAction::triggered, &QtPlayList::slotRemoveActionClicked}
		,{tr("insert"), nullptr, &QAction::triggered, &QtPlayList::slotInsertActionClicked}
		,{tr("add"), nullptr, &QAction::triggered, &QtPlayList::slotAddActionClicked}
	};

protected:
	QtPlayListModel* m_model = nullptr;
	QtNoFocusBorderItemDelegate* m_delegate = nullptr;
	int m_iPlayIndex = -1;
	//int m_iHoverRow = -1;
	std::unique_ptr<std::function<PLAY_MODE()>>m_getPlayModle;
	std::vector<std::tuple<int, QString, QString>>m_randSets;
	std::vector<std::tuple<int, QString, QString>>::const_iterator m_iterRand;
	QtFfplayer* m_pQMediaPlayer = nullptr;

	QLabel* mLabel;
	bool mValidPress;
	int mRowFrom;
	int mRowTo;
	QPoint mDragPoint;
	QPoint mDragPointAtItem;
	int m_iDragRowHeight = -1;
	int m_oldScrollBarV = 0;
	bool m_bIsControlPressed = false;
	bool m_bIsNeedToMakeRandList = true;
	std::set<int>m_vRowAboutToRemove;
	std::set<int>m_vSelectedRow;
	QMenu* m_contextMenu;
	int m_iMenuInsertIndex = -1;
	QTimer* m_timer = nullptr;
	bool m_bScrollToBottom = false;
};
