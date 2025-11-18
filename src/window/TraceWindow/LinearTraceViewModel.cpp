/*

  Copyright (c) 2015, 2016 Hubert Denkmair <hubert@denkmair.de>

  This file is part of cangaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  cangaroo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cangaroo.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "LinearTraceViewModel.h"
#include <iostream>
#include <stddef.h>
#include <core/Backend.h>

LinearTraceViewModel::LinearTraceViewModel(Backend &backend)
  : BaseTraceViewModel(backend)
{
    connect(backend.getTrace(), SIGNAL(beforeAppend(int)), this, SLOT(beforeAppend(int)));
    connect(backend.getTrace(), SIGNAL(afterAppend()), this, SLOT(afterAppend()));
    connect(backend.getTrace(), SIGNAL(beforeClear()), this, SLOT(beforeClear()));
    connect(backend.getTrace(), SIGNAL(afterClear()), this, SLOT(afterClear()));
}

QModelIndex LinearTraceViewModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.internalId()) {
        return createIndex(row, column, (unsigned int)(0x80000000 | parent.internalId()));
    } else {
        return createIndex(row, column, row+1);
    }
}

QModelIndex LinearTraceViewModel::parent(const QModelIndex &child) const
{
    (void) child;
    quintptr id = child.internalId();
    if (id & 0x80000000) {
        return createIndex(id & 0x7FFFFFFF, 0, (unsigned int)(id & 0x7FFFFFFF));
    }
    return QModelIndex();
}

int LinearTraceViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        quintptr id = parent.internalId();
        if (id & 0x80000000) { // node of a message
            return 0;
        } else { 
            const CanMessage *msg = trace()->getMessage(id-1);
            if (msg) {
                CanDbMessage *dbmsg = backend()->findDbMessage(*msg);
                return (dbmsg!=0) ? dbmsg->getSignals().length() : 0;
            } else {
                return 0;
            }
        }
    } else {
        return trace()->size();
    }
}

int LinearTraceViewModel::columnCount(const QModelIndex &parent) const
{
    (void) parent;
    return column_count;
}

bool LinearTraceViewModel::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent)>0;
}

void LinearTraceViewModel::beforeAppend(int num_messages)
{
    beginInsertRows(QModelIndex(), trace()->size(), trace()->size()+num_messages-1);
}

void LinearTraceViewModel::afterAppend()
{
    endInsertRows();
}

void LinearTraceViewModel::beforeClear()
{
    beginResetModel();
}

void LinearTraceViewModel::afterClear()
{
    endResetModel();
}
QVariant LinearTraceViewModel::data_DisplayRole(const QModelIndex &index, int role) const
{
    quintptr internal = index.internalId();
    int msgId = (internal & ~0x80000000u) - 1;

    const CanMessage *msg = trace()->getMessage(msgId);
    if (!msg) return QVariant();

    if (internal & 0x80000000u) {
        return data_DisplayRole_Signal(index, role, *msg);
    }

    if (msgId >= 1) {
        const CanMessage *prev = trace()->getMessage(msgId - 1);
        return data_DisplayRole_Message(index, role, *msg, *prev);
    }

    return data_DisplayRole_Message(index, role, *msg, CanMessage());
}


QVariant LinearTraceViewModel::data_TextColorRole(const QModelIndex &index, int role) const
{
    (void) role;

    quintptr id = index.internalId();

    if (id & 0x80000000) { // CanSignal row
        int msg_id = (id & ~0x80000000)-1;
        const CanMessage *msg = trace()->getMessage(msg_id);
        if (msg) {
            return data_TextColorRole_Signal(index, role, *msg);
        }
    }

    return QVariant();
}

QVariant LinearTraceViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    quintptr internal = index.internalId();
    bool isSignal = (internal & 0x80000000u);
    int msgId = (internal & ~0x80000000u) - 1;

    const CanMessage *msg = trace()->getMessage(msgId);
    if (!msg) return QVariant();

    if (!isSignal && role == Qt::BackgroundRole)
    {
        QColor pastel = messageColorForIdString(msg->getIdString());
        if (pastel.isValid())
            return pastel;
    }

    if (role == Qt::ForegroundRole)
        return data_TextColorRole(index, role);


    if (role == Qt::DisplayRole)
    {
        if (index.column() == column_comment)
        {
            QString perMsg = commentForMessage(msgId);
            if (!perMsg.isEmpty())
                return perMsg;
        }

        return data_DisplayRole(index, role);
    }

    return QVariant();
}



bool LinearTraceViewModel::setData(const QModelIndex &index,
                                   const QVariant &value,
                                   int role)
{
    if (role != Qt::EditRole)
        return false;

    QString text = value.toString().trimmed();
    if (text.isEmpty())
        return false;

    quintptr internal = index.internalId();
    int msgId = (internal & ~0x80000000u) - 1;

    if (msgId < 0 || msgId >= trace()->size())
        return false;

    const CanMessage *msg = trace()->getMessage(msgId);
    if (!msg)
        return false;


    if (index.column() == column_name)
    {
        QString alias = text;
        QString idString = msg->getIdString();

        updateAliasForIdString(idString, alias);

        emit dataChanged(index, index, { Qt::DisplayRole });
        return true;
    }

    if (index.column() == column_comment)
    {
        setCommentForMessage(msgId, text);

        emit dataChanged(index, index, { Qt::DisplayRole });
        return true;
    }

    return false;
}
