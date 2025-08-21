#pragma once
#include <QStyledItemDelegate>

class QProgressSlider;
class QtPlayList;

class QtNoFocusBorderItemDelegate :public QStyledItemDelegate
{
public:
	QtNoFocusBorderItemDelegate(QtPlayList* playList,QProgressSlider* sliderProgress, QWidget*parentWidget);
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

protected:
	QProgressSlider* m_sliderProgress = nullptr;
	QtPlayList* m_listPlayList = nullptr;
};

