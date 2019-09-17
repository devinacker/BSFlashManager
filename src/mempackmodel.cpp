#include "mempackmodel.h"
#include "mempackitem.h"

#include <QIcon>

// ----------------------------------------------------------------------------
MemPackModel::MemPackModel(QObject *parent)
	: QAbstractListModel(parent)
{

}

// ----------------------------------------------------------------------------
int MemPackModel::rowCount(const QModelIndex&) const
{
	return myItems.size();
}

// ----------------------------------------------------------------------------
QVariant MemPackModel::data(const QModelIndex& index, int role) const
{
	static const QIcon validFileIcon(":/res/accept.png");
	static const QIcon deletedFileIcon(":/res/cancel.png");

	int row = index.row();

	if (row < myItems.size())
	{
		const MemPackItem& item = myItems[row];

		if (role == Qt::DisplayRole)
		{
			return tr("%1\t%2/%3\n%4 blocks\n%5")
				.arg(item.title)
				.arg(item.header.month >> 4).arg(item.header.day >> 3)
				.arg(item.blocks)
				.arg(item.starts >= 0 ? tr("%1 play(s) left").arg(item.starts) : tr("Unlimited plays"));
		}
		else if (role == Qt::DecorationRole)
		{
			if (item.deleted) return deletedFileIcon;
			else return validFileIcon;
		}
	}
	
	return QVariant();
}

// ----------------------------------------------------------------------------
bool MemPackModel::removeRows(int row, int count, const QModelIndex &parent)
{
	if (row < 0 || count < 1) return false;

	beginRemoveRows(parent, row, row + count - 1);
	while (count--)
	{
		if (row >= 0 && row < myItems.count())
		{
			myItems.removeAt(row);
		}
	}
	endRemoveRows();
	
	return true;
}

// ----------------------------------------------------------------------------
bool MemPackModel::moveRowUp(int row)
{
	if (row <= 0 || row >= myItems.count()) return false;

	static const QModelIndex dummy;
	if (beginMoveRows(dummy, row, row, dummy, row - 1))
	{
		myItems.move(row, row - 1);
		endMoveRows();
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
bool MemPackModel::moveRowDown(int row)
{
	if (row < 0 || row + 1 >= myItems.count()) return false;

	static const QModelIndex dummy;
	if (beginMoveRows(dummy, row, row, dummy, row + 2))
	{
		myItems.move(row, row + 1);
		endMoveRows();
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
MemPackItems& MemPackModel::items()
{
	return myItems;
}

// ----------------------------------------------------------------------------
void MemPackModel::setItems(const MemPackItems& newItems)
{
	beginResetModel();
	myItems = newItems;
	endResetModel();
}

// ----------------------------------------------------------------------------
ItemHeader MemPackModel::itemHeader(unsigned num) const
{
	if (num < (unsigned)myItems.size())
	{
		return myItems[num].header;
	}

	return ItemHeader();
}

// ----------------------------------------------------------------------------
void MemPackModel::setItemHeader(unsigned num, const ItemHeader& header)
{
	if (num < (unsigned)myItems.size())
	{
		myItems[num].header = header;
		QModelIndex index = this->index(num, 0);
		emit dataChanged(index, index);
	}
}
