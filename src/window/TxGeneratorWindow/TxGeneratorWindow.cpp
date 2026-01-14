
/*

  Copyright (c) 2024 Schildkroet

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

#include "TxGeneratorWindow.h"
#include "core/MeasurementNetwork.h"
#include "ui_TxGeneratorWindow.h"

#include <QDomDocument>
#include <QTimer>
#include <QInputDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDateTime>

#include <core/Backend.h>
#include <driver/CanInterface.h>
#include <core/CanTrace.h>
#include "window/RawTxWindow/RawTxWindow.h"
#include "helpers/apphelpers.h"
using namespace AppHelpers;

TxGeneratorWindow::TxGeneratorWindow(QWidget *parent, Backend &backend)
    : ConfigurableWidget(parent),
      ui(new Ui::TxGeneratorWindow),
      _backend(backend)
{
    ui->setupUi(this);

    ui->treeWidget->setHeaderLabels(QStringList() << tr("Nr") << tr("Name") << tr("Cycle Time") << tr("Channel"));
    ui->treeWidget->setColumnWidth(0, 40);
    ui->treeWidget->setColumnWidth(1, 160);

    if (ui->btnRemove)
        ui->btnRemove->setEnabled(true);

    _SendTimer = new QTimer(this);
    _SendTimer->setTimerType(Qt::PreciseTimer);
    _SendTimer->setInterval(5);
    connect(_SendTimer, SIGNAL(timeout()), this, SLOT(SendTimer_timeout()));
    _SendTimer->start();

    connect(&_backend, SIGNAL(beginMeasurement()), this, SLOT(update()));
    connect(&_backend, SIGNAL(endMeasurement()), this, SLOT(update()));

    update();
}

TxGeneratorWindow::~TxGeneratorWindow()
{
    delete ui;
}

bool TxGeneratorWindow::saveXML(Backend &backend, QDomDocument &xml, QDomElement &root)
{
    if (!ConfigurableWidget::saveXML(backend, xml, root))
    {
        return false;
    }
    root.setAttribute("type", "TxGeneratorWindow");
    return true;
}

bool TxGeneratorWindow::loadXML(Backend &backend, QDomElement &el)
{
    if (!ConfigurableWidget::loadXML(backend, el))
    {
        return false;
    }
    return true;
}

void TxGeneratorWindow::SendTimer_timeout()
{
    if (_tasks.isEmpty())
        return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    for (int i = 0; i < _tasks.size(); ++i)
    {
        TxTask &t = _tasks[i];

        if (!t.enabled)
            continue;

        if (now - t.last_sent >= t.period_ms)
        {
            CanInterface *intf = _backend.getInterfaceById(t.msg.getInterfaceId());

            if (intf && intf->isOpen())
            {
                if (t.msg.getInterfaceId() == 0 && intf)
                    t.msg.setInterfaceId(intf->getId());


                qint64 msec = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
                t.msg.setTimestamp({
                    static_cast<long>(msec / 1000),        // Sekunden
                    static_cast<long>((msec % 1000) * 1000) // Mikrosekunden
                });

            
                t.msg.setRX(false);
                t.msg.setShow(true);

                intf->sendMessage(t.msg);
                _backend.getTrace()->enqueueMessage(t.msg);

                // qDebug() << "[TxGenerator] TX id=" << Qt::hex << t.msg.getId()
                //          << "period=" << t.period_ms;
            }

            t.last_sent = now;
        }
    }
}

void TxGeneratorWindow::update()
{
    auto list = _backend.getInterfaceList();
    if (list.count() == 0)
    {
        this->setDisabled(true);
        return;
    }

    bool anyOpen = false;
    foreach (CanInterfaceId ifid, list)
    {
        auto intf = _backend.getInterfaceById(ifid);
        if (intf && intf->isOpen())
        {
            anyOpen = true;
            break;
        }
    }

    this->setDisabled(!anyOpen);
}

void TxGeneratorWindow::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (!item)
        return;

    int index = item->text(column_nr).toInt() - 1;
    if (index < 0 || index >= _tasks.size())
        return;

    TxTask &t = _tasks[index];

    ui->btnRemove->setEnabled(true);

    ui->btnEnable->setEnabled(!t.enabled);

    ui->btnDisable->setEnabled(t.enabled);
}

void TxGeneratorWindow::on_btnAdd_released()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Add periodic CAN message"));

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    RawTxWindow *editor = new RawTxWindow(&dlg, _backend);
    editor->refreshInterfaces();
    editor->setDialogMode(true);

    layout->addWidget(editor);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        &dlg);
    layout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    //dlg.setLayout(layout);
    //dlg.adjustSize();

    if (dlg.exec() != QDialog::Accepted)
    {
        qDebug() << "[TxGenerator] cancelled by user";
        return;
    }

    CanMessage msg;
    editor->getCurrentMessage(msg);

    editor->setTaskEditMode(true);

    int period = editor->getPeriodMs();

    TxTask t;
    t.msg = msg;
    t.period_ms = period;
    t.last_sent = 0;
    _tasks.append(t);

    QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(column_nr, QString::number(_tasks.count()));
    item->setText(column_name, msg.getDataHexString());
    item->setText(column_cycletime, QString::number(period));
    item->setText(column_channel, _backend.getInterfaceName(msg.getInterfaceId()));
    ui->treeWidget->addTopLevelItem(item);
    setRowColor(item, true);
    if (ui->btnRemove)
        ui->btnRemove->setEnabled(true);

    // qDebug() << "[TxGenerator] Added periodic task id=" << msg.getId()
    //          << "period=" << period;
}

void TxGeneratorWindow::on_btnRemove_released()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item)
        return;

    int index = item->text(column_nr).toInt() - 1;
    if (index < 0 || index >= _tasks.size())
        return;

    _tasks.removeAt(index);
    delete item;

    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
    {
        ui->treeWidget->topLevelItem(i)->setText(column_nr, QString::number(i + 1));
    }

    if (ui->treeWidget->topLevelItemCount() == 0 && ui->btnRemove)
        ui->btnRemove->setEnabled(false);

    // qDebug() << "[TxGenerator] Removed task index=" << index;
}

void TxGeneratorWindow::on_btnEnable_released()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item)
        return;

    int index = item->text(column_nr).toInt() - 1;
    if (index < 0 || index >= _tasks.size())
        return;

    _tasks[index].enabled = true;
    setRowColor(ui->treeWidget->currentItem(), true);
    ui->btnEnable->setEnabled(false);
    ui->btnDisable->setEnabled(true);

    // qDebug() << "[TxGenerator] Enabled task" << index;
}

void TxGeneratorWindow::on_btnDisable_released()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item)
        return;

    int index = item->text(column_nr).toInt() - 1;
    if (index < 0 || index >= _tasks.size())
        return;

    _tasks[index].enabled = false;
    setRowColor(ui->treeWidget->currentItem(), false);
    ui->btnEnable->setEnabled(true);
    ui->btnDisable->setEnabled(false);

    // qDebug() << "[TxGenerator] Disabled task" << index;
}

void TxGeneratorWindow::retranslateUi() {
    ui->retranslateUi(this);
}
