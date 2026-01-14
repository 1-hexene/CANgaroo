

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
#pragma once

#include <core/Backend.h>
#include <core/ConfigurableWidget.h>
#include <core/MeasurementSetup.h>
#include <QList>
#include <QTreeWidgetItem>

namespace Ui
{
    class TxGeneratorWindow;
}

class QDomDocument;
class QDomElement;

class TxGeneratorWindow : public ConfigurableWidget
{
    Q_OBJECT
public:
    explicit TxGeneratorWindow(QWidget *parent, Backend &backend);
    ~TxGeneratorWindow();

    virtual bool saveXML(Backend &backend, QDomDocument &xml, QDomElement &root);
    virtual bool loadXML(Backend &backend, QDomElement &el);

    enum
    {
        column_nr = 0,
        column_name,
        column_cycletime,
        column_channel
    };

private slots:
    void SendTimer_timeout();
    void update();

    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_btnAdd_released();
    void on_btnRemove_released();
    void on_btnEnable_released();
    void on_btnDisable_released();

protected:
    struct TxTask
    {
        CanMessage msg;
        int period_ms;
        qint64 last_sent;
        bool enabled=true;
    };
    QList<TxTask> _tasks;

    void retranslateUi() override;

private:
    Ui::TxGeneratorWindow *ui;
    Backend &_backend;
    QTimer *_SendTimer;
};
