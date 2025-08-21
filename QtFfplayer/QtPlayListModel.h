#pragma once
#include <qabstractitemmodel.h>
#include<vector>
#include<utility>
class QMimeData;
class QtPlayList;

class QtPlayListModel :public QAbstractItemModel
{
public:
	QtPlayListModel(QtPlayList*pQtPlayList,QWidget* pParentWidget = nullptr);
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex& child) const override;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role) const override;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole)override;
    std::vector<std::pair<QString, QString>>getData();
    void insertData(int posRow, QStringList&&dataSets);
    void setDatas(QStringList&& dataSets);
    bool removeRows(int position, int rows, const QModelIndex& index);
    bool removeRow(int row, const QModelIndex& parent = QModelIndex());

    std::vector<QString>getAllPathFilesInList();

protected:
	std::vector<std::pair<QString, QString>>m_vData;
    QtPlayList* m_pQtPlayList;
};

