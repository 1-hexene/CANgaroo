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

#include "BaseTraceViewModel.h"
#include "qtooltip.h"

#include <QDateTime>
#include <QColor>
#include <core/MeasurementNetwork.h>
#include <core/CanDb.h>
#include <core/Backend.h>
#include <core/CanTrace.h>
#include <core/CanMessage.h>
#include <core/CanDbMessage.h>
#include <iostream>

BaseTraceViewModel::BaseTraceViewModel(Backend &backend)
{
    _backend = &backend;
}

int BaseTraceViewModel::columnCount(const QModelIndex &parent) const
{
    (void)parent;
    return column_count;
}

QVariant BaseTraceViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {

        if (orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case column_index:
                return QString(tr("Index"));
            case column_timestamp:
                return QString(tr("Timestamp"));
            case column_channel:
                return QString(tr("Channel"));
            case column_direction:
                return QString(tr("RX/TX"));
            case column_type:
                return QString(tr("Type"));
            case column_canid:
                return QString("ID");
            case column_sender:
                return QString(tr("Sender"));
            case column_name:
                return QString(tr("Name"));
            case column_dlc:
                return QString("DLC");
            case column_data:
                return QString(tr("Data"));
            case column_comment:
                return QString(tr("Comment"));
            }
        }
    }
    else if (role == Qt::TextAlignmentRole)
    {
        switch (section)
        {
        case column_index:
            return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
        case column_timestamp:
            return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
        case column_channel:
            return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
        case column_direction:
            return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
        case column_type:
            return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
        case column_canid:
            return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
        case column_sender:
            return static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter);
        case column_name:
            return static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter);
        case column_dlc:
            return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
        case column_data:
            return static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter);
        case column_comment:
            return static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter);
        default:
            return QVariant();
        }
    }
    return QVariant();
}

QVariant BaseTraceViewModel::data(const QModelIndex &index, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
        return data_DisplayRole(index, role);
    case Qt::TextAlignmentRole:
        return data_TextAlignmentRole(index, role);
    case Qt::ForegroundRole:
        return data_TextColorRole(index, role);
    case Qt::ToolTipRole:
    {
        QString data = index.data(Qt::DisplayRole).toString();
        uint length = data.length();
        if (length > 24)
        {
            uint div = length / 24;
            for (uint i = 0; i < div - 1; i++)
            {
                if ((i + 1) % 2 == 0 || index.column() != column_data)
                    data.insert(24 * (i + 1) + i, "\n");
                else
                    data.insert(24 * (i + 1) + i, " ");
            }
        }
        return data;
    }
        // return QString("Row%1, Column%2").arg(row + 1).arg(col +1);
    default:
        return QVariant();
    }
}

Backend *BaseTraceViewModel::backend() const
{
    return _backend;
}

CanTrace *BaseTraceViewModel::trace() const
{
    return _backend->getTrace();
}

timestamp_mode_t BaseTraceViewModel::timestampMode() const
{
    return _timestampMode;
}

void BaseTraceViewModel::setTimestampMode(timestamp_mode_t timestampMode)
{
    _timestampMode = timestampMode;
}

QVariant BaseTraceViewModel::formatTimestamp(timestamp_mode_t mode, const CanMessage &currentMsg, const CanMessage &lastMsg) const
{

    if (mode == timestamp_mode_delta)
    {

        double t_current = currentMsg.getFloatTimestamp();
        double t_last = lastMsg.getFloatTimestamp();
        if (t_last == 0)
        {
            return QString().asprintf("%.04lf", 0.00);
        }
        else
        {
            return QString().asprintf("%.04lf", t_current - t_last);
        }
    }
    else if (mode == timestamp_mode_absolute)
    {

        return currentMsg.getDateTime().toString("hh:mm:ss.zzz");
    }
    else if (mode == timestamp_mode_relative)
    {

        double t_current = currentMsg.getFloatTimestamp();
        return QString().asprintf("%.04lf", t_current - backend()->getTimestampAtMeasurementStart());
    }

    return QVariant();
}

QVariant BaseTraceViewModel::data_DisplayRole(const QModelIndex &index, int role) const
{
    (void)index;
    (void)role;
    return QVariant();
}

QVariant BaseTraceViewModel::data_DisplayRole_Message(const QModelIndex &index, int role, const CanMessage &currentMsg, const CanMessage &lastMsg) const
{
    (void)role;
    CanDbMessage *dbmsg = backend()->findDbMessage(currentMsg);

    switch (index.column())
    {

    case column_index:
        return index.internalId();

    case column_timestamp:
        return formatTimestamp(_timestampMode, currentMsg, lastMsg);

    case column_channel:
        return backend()->getInterfaceName(currentMsg.getInterfaceId());

    case column_direction:
        return currentMsg.isRX() ? "RX" : "TX";

    case column_type:
    {
        QString _type = QString(currentMsg.isFD() ? "FD." : "") + QString(currentMsg.isExtended() ? "EXT." : "STD.") + QString(currentMsg.isRTR() ? "RTR" : "") + QString((currentMsg.isBRS() ? "BRS" : ""));
        return _type;
    }

    case column_canid:
        return currentMsg.getIdString();

    case column_name:
        return (dbmsg) ? dbmsg->getName() : "";

    case column_sender:
    {
        if (!dbmsg)
        {
            return "";
        }
        auto sender = dbmsg->getSender();
        if (!sender)
        {
            return "";
        }
        return sender->name();
    }

    case column_dlc:
        return currentMsg.getLength();

    case column_data:
        return currentMsg.getDataHexString();

    case column_comment:
        return (dbmsg) ? dbmsg->getComment() : "";

    default:
        return QVariant();
    }
}

QVariant BaseTraceViewModel::data_DisplayRole_Signal(const QModelIndex &index, int role, const CanMessage &msg) const
{
    (void)role;
    uint64_t raw_data;
    QString value_name;
    QString unit;

    CanDbMessage *dbmsg = backend()->findDbMessage(msg);
    if (!dbmsg)
    {
        return QVariant();
    }

    CanDbSignal *dbsignal = dbmsg->getSignal(index.row());
    if (!dbsignal)
    {
        return QVariant();
    }

    switch (index.column())
    {

    case column_name:
        return dbsignal->name();

    case column_data:

        if (dbsignal->isPresentInMessage(msg))
        {
            raw_data = dbsignal->extractRawDataFromMessage(msg);
        }
        else
        {
            if (!trace()->getMuxedSignalFromCache(dbsignal, &raw_data))
            {
                return QVariant();
            }
        }

        value_name = dbsignal->getValueName(raw_data);
        if (value_name.isEmpty())
        {
            unit = dbsignal->getUnit();
            if (unit.isEmpty())
            {
                return dbsignal->convertRawValueToPhysical(raw_data);
            }
            else
            {
                return QString("%1 %2").arg(dbsignal->convertRawValueToPhysical(raw_data)).arg(unit);
            }
        }
        else
        {
            return QString("%1 - %2").arg(raw_data).arg(value_name);
        }

    case column_comment:
        return dbsignal->comment().replace('\n', ' ');

    default:
        return QVariant();
    }
}

QVariant BaseTraceViewModel::data_TextAlignmentRole(const QModelIndex &index, int role) const
{
    (void)role;
    switch (index.column())
    {
    case column_index:
        return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
    case column_timestamp:
        return static_cast<int>(Qt::AlignRight) + static_cast<int>(Qt::AlignVCenter);
    case column_channel:
        return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
    case column_direction:
        return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
    case column_type:
        return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
    case column_canid:
        return static_cast<int>(Qt::AlignRight) + static_cast<int>(Qt::AlignVCenter);
    case column_sender:
        return static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter);
    case column_name:
        return static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter);
    case column_dlc:
        return static_cast<int>(Qt::AlignCenter) + static_cast<int>(Qt::AlignVCenter);
    case column_data:
        return static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter);
    case column_comment:
        return static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter);
    default:
        return QVariant();
    }
}

QVariant BaseTraceViewModel::data_TextColorRole(const QModelIndex &index, int role) const
{
    (void)role;

    if (!index.isValid()) {
        return QVariant();
    }

    if (index.parent().isValid()) {
        return QVariant();
    }

    QModelIndex idIndex = this->index(index.row(),
                                      column_canid,
                                      index.parent());
    QString idString = idIndex.data(Qt::DisplayRole).toString();
    if (idString.isEmpty()) {
        return QVariant();
    }

    auto it = _idColors.constFind(idString);
    if (it == _idColors.constEnd()) {
        return QVariant();
    }

   
    return *it;
}

QVariant BaseTraceViewModel::data_TextColorRole_Signal(const QModelIndex &index, int role, const CanMessage &msg) const
{
    (void)role;

    CanDbMessage *dbmsg = backend()->findDbMessage(msg);
    if (!dbmsg)
    {
        return QVariant();
    }

    CanDbSignal *dbsignal = dbmsg->getSignal(index.row());
    if (!dbsignal)
    {
        return QVariant();
    }

    if (dbsignal->isPresentInMessage(msg))
    {
        return QVariant();
    }
    else
    {
        return QVariant::fromValue(QColor(200, 200, 200));
    }
}
Qt::ItemFlags BaseTraceViewModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.parent().isValid())
    {
        return defaultFlags;
    }

    if (index.column() == column_name || index.column() == column_comment)
    {
        return defaultFlags;
    }

    return defaultFlags;
}
bool BaseTraceViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || value.toString().isEmpty())
    {
        return false;
    }
    quintptr internal = index.internalId();
    int msg_id = (internal & ~0x80000000u) - 1;
    if (msg_id < 0 || msg_id >= trace()->size()) {
        return false;
    }

    const CanMessage *msg = trace()->getMessage(msg_id);
    if (!msg)
    {
        return false;
    }

    MeasurementNetwork *network = backend()->getSetup().getNetwork(0);
    if (!network)
    {
        return false;
    }

    pCanDb db;
    if (network->_canDbs.isEmpty())
    {
        db = pCanDb(new CanDb());
        network->addCanDb(db);
    }
    else
    {
        db = network->_canDbs.first();
    }

    CanDbMessage *dbmsg = backend()->findDbMessage(*msg);

    if (!dbmsg)
    {
        dbmsg = new CanDbMessage(db.data());
        dbmsg->setRaw_id(msg->getRawId());
        dbmsg->setDlc(msg->getLength());

        db->addMessage(dbmsg);
    }

    if (index.column() == column_name)
    {
        dbmsg->setName(value.toString());
    }
    else if (index.column() == column_comment)
    {
        dbmsg->setComment(value.toString());
    }
    else
    {
        return false;
    }

    QModelIndex firstColIndex = index.sibling(index.row(), 0);
    QModelIndex lastColIndex = index.sibling(index.row(), column_count - 1);
    emit dataChanged(firstColIndex, lastColIndex);

    return true;
}

void BaseTraceViewModel::updateAliasForIdString(const QString &idString,
                                                const QString &alias)
{
    CanTrace *t = trace();
    if (!t) {
        return;
    }

    const CanMessage *exampleMsg = nullptr;
    int sz = t->size();
    for (int i = 0; i < sz; ++i) {
        const CanMessage *m = t->getMessage(i);
        if (!m) {
            continue;
        }
        if (m->getIdString() == idString) {
            exampleMsg = m;
            break;
        }
    }

    if (!exampleMsg) {
        return;
    }

    MeasurementNetwork *network = backend()->getSetup().getNetwork(0);
    if (!network) {
        return;
    }

    pCanDb db;
    if (network->_canDbs.isEmpty()) {
        db = pCanDb(new CanDb());
        network->addCanDb(db);
    } else {
        db = network->_canDbs.first();
    }

    CanDbMessage *dbmsg = backend()->findDbMessage(*exampleMsg);
    if (!dbmsg) {
        dbmsg = new CanDbMessage(db.data());
        dbmsg->setRaw_id(exampleMsg->getRawId());
        dbmsg->setDlc(exampleMsg->getLength());
        db->addMessage(dbmsg);
    }

    dbmsg->setName(alias);

    int rows = rowCount(QModelIndex());
    if (rows > 0) {
        QModelIndex topLeft =
            index(0, column_name, QModelIndex());
        QModelIndex bottomRight =
            index(rows - 1, column_name, QModelIndex());
        emit dataChanged(topLeft, bottomRight, { Qt::DisplayRole });
    }
}

void BaseTraceViewModel::setMessageColorForIdString(const QString &idString,
                                                    const QColor &color)
{
    _idColors.insert(idString, color);

    int rows = rowCount(QModelIndex());
    if (rows <= 0) {
        return;
    }

    QModelIndex topLeft = index(0, 0, QModelIndex());
    QModelIndex bottomRight = index(rows - 1, column_count - 1, QModelIndex());
    emit dataChanged(topLeft, bottomRight, { Qt::ForegroundRole });
}

QColor BaseTraceViewModel::messageColorForIdString(const QString &idString) const
{
    return _idColors.value(idString, QColor());
}

QColor BaseTraceViewModel::getColorForId(quint32 rawId) const
{
    QString key = QStringLiteral("0x%1").arg(rawId, 8, 16, QLatin1Char('0')).toUpper();
    return _idColors.value(key, QColor());
}

QString BaseTraceViewModel::commentForMessage(int msgId) const
{
    return _perMessageComment.value(msgId, "");
}

void BaseTraceViewModel::setCommentForMessage(int msgId, const QString &c)
{
    _perMessageComment[msgId] = c;
}