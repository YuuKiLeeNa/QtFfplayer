#include "QtNoFocusBorderItemDelegate.h"
#include<QPainter>
#include"QProgressSlider.h"
#include<cmath>
#include "QtPlayList.h"
#include<QDebug>

QtNoFocusBorderItemDelegate::QtNoFocusBorderItemDelegate(QtPlayList* playList, QProgressSlider* sliderProgress, QWidget* parentWidget):QStyledItemDelegate(parentWidget), m_sliderProgress(sliderProgress), m_listPlayList(playList)
{
}

void QtNoFocusBorderItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem  view_option(option);
    if (view_option.state & QStyle::State_HasFocus) {
        view_option.state = view_option.state ^ QStyle::State_HasFocus;
    }
    std::set<int>v = m_listPlayList->getSelectedRowSets();
    if (m_listPlayList->getCurPlayIndexFile().second == index.row())
    {
        long long iMin = m_sliderProgress->minimum();
        long long iMax = m_sliderProgress->maximum();
        long long iv = m_sliderProgress->value();

        if (iMax != iMin)
        {
            long double ratio = (long double)iv / std::abs(iMax - iMin);
            view_option.rect.adjust(2,2,-2,-2);
            QPoint ptBeg = view_option.rect.topLeft();
            QPoint ptEnd = view_option.rect.bottomRight();
            ptEnd.setX((ptEnd.x() - ptBeg.x()) * ratio);
            QLinearGradient linear(ptBeg, ptEnd);
            linear.setColorAt(0, QColor(0x66, 0xCD, 0xAA, 00));
            linear.setColorAt(1, QColor(0x66, 0xCD, 0xAA, 0xB0));
            painter->fillRect(QRect(ptBeg, ptEnd), linear);
        }
        if (v.find(index.row()) != v.end())
        {
            if (!(option.state & QStyle::State_MouseOver))
            {
                painter->save();
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(Qt::white));
                painter->drawText(view_option.rect, index.data(Qt::TextAlignmentRole).toInt(), index.data(Qt::DisplayRole).toString());
                painter->restore();
            }
            else
            {
                painter->save();
                painter->setPen(QPen(QColor(0x4F, 0x94, 0xCD)));
                painter->setBrush(Qt::NoBrush);
                view_option.rect.adjust(4, 2, -2, -2);
                painter->drawText(view_option.rect, index.data(Qt::TextAlignmentRole).toInt(), index.data(Qt::DisplayRole).toString());
                view_option.rect.adjust(-2, 0, 0, 0);
                painter->drawRect(view_option.rect);
                painter->restore();
            }
        }
        else if (option.state & QStyle::State_MouseOver)
        {
            painter->save();
            painter->setPen(QPen(QColor(0x4F, 0x94, 0xCD)));
            painter->setBrush(Qt::NoBrush);
            view_option.rect.adjust(4, 2, -2, -2);
            painter->drawText(view_option.rect, index.data(Qt::TextAlignmentRole).toInt(), index.data(Qt::DisplayRole).toString());
            view_option.rect.adjust(-2, 0, 0, 0);
            painter->drawRect(view_option.rect);
            painter->restore();
        }
        else
            QStyledItemDelegate::paint(painter, view_option, index);
    }
    else if (v.find(index.row()) != v.end())
    {
        painter->fillRect(view_option.rect, QColor(180, 200, 220, 0x80));
        if (!(option.state & QStyle::State_MouseOver))
        {
            painter->save();
            painter->setPen(QPen(Qt::white));
            painter->drawText(view_option.rect, index.data(Qt::TextAlignmentRole).toInt(), index.data(Qt::DisplayRole).toString());
            painter->restore();
        }
        else 
        {
            painter->save();
            painter->setPen(QPen(QColor(0x4F, 0x94, 0xCD)));
            painter->setBrush(Qt::NoBrush);
            view_option.rect.adjust(4, 2, -2, -2);
            painter->drawText(view_option.rect, index.data(Qt::TextAlignmentRole).toInt(), index.data(Qt::DisplayRole).toString());
            view_option.rect.adjust(-2, 0, 0, 0);
            painter->drawRect(view_option.rect);
            painter->restore();
        }
    
    }
    else if (option.state & QStyle::State_MouseOver)
    {
        painter->save();
        painter->setPen(QPen(QColor(0x4F,0x94,0xCD)));
        painter->setBrush(Qt::NoBrush);

        view_option.rect.adjust(4,2,-2,-2);
        painter->drawText(view_option.rect, index.data(Qt::TextAlignmentRole).toInt(), index.data(Qt::DisplayRole).toString());
        view_option.rect.adjust(-2, 0, 0, 0);
        painter->drawRect(view_option.rect);
        painter->restore();

    }
    else
        QStyledItemDelegate::paint(painter, view_option, index);

}

