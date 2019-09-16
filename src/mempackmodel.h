#pragma once

#include <QAbstractListModel>
#include "mempackitem.h"

class MemPackModel : public QAbstractListModel
{
	Q_OBJECT

public:
	MemPackModel(QObject *parent = nullptr);
	~MemPackModel() {}

	int rowCount(const QModelIndex& = QModelIndex()) const;
	QVariant data(const QModelIndex&, int = Qt::DisplayRole) const;

	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
	bool moveRowUp(int row);
	bool moveRowDown(int row);

	MemPackItems& items();
	void setItems(const MemPackItems&);

	ItemHeader itemHeader(unsigned) const;
	void setItemHeader(unsigned, const ItemHeader&);

private:
	MemPackItems myItems;
};
