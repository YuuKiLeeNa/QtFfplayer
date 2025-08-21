#include "QtPlayListModel.h"
#include<QColor>
#include<QWidget>
#include<iterator>
#include "QtPlayList.h"
#include<QMimeData>
#include "CConfig.h"
#include<QAbstractItemModel>
#include<qnamespace.h>

QtPlayListModel::QtPlayListModel(QtPlayList* pQtPlayList,QWidget* pParentWidget):QAbstractItemModel(pParentWidget), m_pQtPlayList(pQtPlayList)
{
}

QModelIndex QtPlayListModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
    return createIndex(row, column);
}
QModelIndex QtPlayListModel::parent(const QModelIndex& child) const
{
    return QModelIndex();
}
int QtPlayListModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    if (parent.row() == -1)
        return m_vData.size();
    return 0;
}
int QtPlayListModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return 1;
}

QVariant QtPlayListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    if (row < 0 || row >= m_vData.size() || index.column() > 0)
        return QVariant();

    //int column = index.column();
    //QString name = m_vData[row].second;
    switch (role) {
    case Qt::BackgroundRole://Qt::BackgroundColorRole: 
    {
        //    return QColor(180, 200, 220, 0xA0);
        return QColor(0,0,0,0);
    }
    case Qt::DecorationRole: {
         return QVariant();
    }
    case Qt::DisplayRole: {
        int column = index.column();
        return m_vData[row].second;
    }
    case Qt::TextAlignmentRole: {
        return QVariant(Qt::AlignLeft  | Qt::AlignVCenter);
    }
    case Qt::FontRole://TextColorRole:
    {
        //    return QColor(0xA2, 0xCD, 0x5A);
        return QColor(255, 255, 255, 0xA0);
    }
    case Qt::ToolTipRole:
    {
        return m_vData[row].first;
    }

    default:
        return QVariant();
    }
    return QVariant();
}

bool QtPlayListModel::setData(const QModelIndex& index, const QVariant& value, int role) 
{
    switch (role)
    {
    case Qt::DisplayRole:
        //QString str = value.toString();
        m_vData[index.row()] = { value.toString() , QtPlayList::getFileName(value.toString())};
        return true;
    default:
        break;
    }
    return false;
}

std::vector<std::pair<QString, QString>>QtPlayListModel::getData()
{
    return m_vData;
}

void QtPlayListModel::insertData(int posRow, QStringList&& dataSets) 
{
    int iDataSize = dataSets.size();
    if (iDataSize == 0)
        return;
    m_pQtPlayList->setNeedToMakeRandList();
    beginInsertRows(QModelIndex(), posRow, posRow + iDataSize - 1);
    m_vData.reserve(m_vData.size()+ dataSets.size());
    std::insert_iterator<decltype(m_vData)>INSERT(m_vData, m_vData.begin()+posRow);
    for (auto& ele : dataSets)
        *INSERT++ = { ele , QtPlayList::getFileName(ele) };
    endInsertRows();

    CConfig::writePlayList(getAllPathFilesInList());
}

void QtPlayListModel::setDatas(QStringList&& dataSets) 
{
    int isize = m_vData.size();
    if (isize > 0)
    {
        beginRemoveRows(QModelIndex(), 0, isize - 1);
        endRemoveRows();
    }
    insertData(0,std::move(dataSets));
    m_vData.shrink_to_fit();
    CConfig::writePlayList(getAllPathFilesInList());
}

bool QtPlayListModel::removeRows(int position, int rows, const QModelIndex& index)
{
    m_pQtPlayList->setNeedToMakeRandList();
    beginRemoveRows(index, position, position + rows - 1);
    m_vData.erase(m_vData.begin()+ position, m_vData.begin() + position + rows);
    endRemoveRows();
    CConfig::writePlayList(getAllPathFilesInList());
    return true;
}

bool QtPlayListModel::removeRow(int row, const QModelIndex& parent)
{
    m_pQtPlayList->setNeedToMakeRandList();
    beginRemoveRows(parent, row, row);
    m_vData.erase(m_vData.begin() + row);
    endRemoveRows();
    CConfig::writePlayList(getAllPathFilesInList());
    return true;
}

std::vector<QString>QtPlayListModel::getAllPathFilesInList()
{
    auto sets = getData();
    std::vector<QString>resSets;
    resSets.reserve(sets.size());
    for (auto& ele : sets)
        resSets.push_back(ele.first);
    return resSets;
}