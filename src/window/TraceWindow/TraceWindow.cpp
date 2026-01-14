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

#include "TraceWindow.h"
#include "ui_TraceWindow.h"

#include <QDomDocument>
#include <QSortFilterProxyModel>
#include "LinearTraceViewModel.h"
#include "AggregatedTraceViewModel.h"
#include "TraceFilterModel.h"
#include <core/Backend.h>

#include <QInputDialog>
#include <QColorDialog>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>   
#include <QFrame>
#include "helpers/apphelpers.h"
using namespace AppHelpers;


TraceWindow::TraceWindow(QWidget *parent, Backend &backend) : ConfigurableWidget(parent),
                                                              ui(new Ui::TraceWindow),
                                                              _backend(&backend),
                                                              _mode(mode_linear),
                                                              _doAutoScroll(false),
                                                              _timestampMode(timestamp_mode_absolute)
{
    ui->setupUi(this);

    _linearTraceViewModel = new LinearTraceViewModel(backend);
    _linearProxyModel = new QSortFilterProxyModel(this);
    _linearProxyModel->setSourceModel(_linearTraceViewModel);
    _linearProxyModel->setDynamicSortFilter(true);

    _aggregatedTraceViewModel = new AggregatedTraceViewModel(backend);
    _aggregatedProxyModel = new QSortFilterProxyModel(this);
    _aggregatedProxyModel->setSourceModel(_aggregatedTraceViewModel);
    _aggregatedProxyModel->setDynamicSortFilter(true);

    _aggFilteredModel = new TraceFilterModel(this);
    _aggFilteredModel->setSourceModel(_aggregatedProxyModel);
    _linFilteredModel = new TraceFilterModel(this);
    _linFilteredModel->setSourceModel(_linearProxyModel);

    setMode(mode_aggregated);
    setAutoScroll(false);

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    ui->tree->setFont(font);
    ui->tree->setAlternatingRowColors(true);

    ui->tree->setUniformRowHeights(true);

    ui->tree->setColumnWidth(BaseTraceViewModel::column_comment, 140);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_timestamp, 120);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_channel, 120);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_direction, 70);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_type, 70);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_canid, 120);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_sender, 150);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_name, 150);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_dlc, 50);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_data, 160);

    ui->tree->sortByColumn(BaseTraceViewModel::column_index, Qt::AscendingOrder);

    ui->cbTimestampMode->addItem(tr("Absolute"), 0);
    ui->cbTimestampMode->addItem(tr("Relative"), 1);
    ui->cbTimestampMode->addItem(tr("Delta"), 2);
    setTimestampMode(timestamp_mode_delta);

    connect(_linearTraceViewModel, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(rowsInserted(QModelIndex, int, int)));
    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(on_cbFilterChanged()));
    connect(ui->TraceClearpushButton, SIGNAL(released()), this, SLOT(on_cbTraceClearpushButton()));
    connect(ui->cbAggregated, SIGNAL(stateChanged(int)), this, SLOT(on_cbAggregated_stateChanged(int)));
    connect(ui->cbAutoScroll, SIGNAL(stateChanged(int)), this, SLOT(on_cbAutoScroll_stateChanged(int)));

    connect(ui->tree, &QTreeView::doubleClicked, this, &TraceWindow::onTraceRowDoubleClicked);

    ui->cbAggregated->setCheckState(Qt::Unchecked);
    ui->cbAutoScroll->setCheckState(Qt::Checked);
}

TraceWindow::~TraceWindow()
{
    delete ui;
    delete _aggregatedTraceViewModel;
    delete _linearTraceViewModel;
}
void TraceWindow::setMode(TraceWindow::mode_t mode)
{
    // qDebug() << "TraceWindow::setMode from"
    //          << (_mode == mode_linear ? "linear" : "aggregated")
    //          << "to"
    //          << (mode == mode_linear ? "linear" : "aggregated");

    // qDebug() << "  [Before] Linear aliases:"
    //          << _linearTraceViewModel->getAllIdAliases();
    // qDebug() << "  [Before] Aggregated aliases:"
    //          << _aggregatedTraceViewModel->getAllIdAliases();

    bool isChanged = (_mode != mode);
    _mode = mode;

    if (_mode == mode_linear)
    {
        ui->tree->setSortingEnabled(false);
        ui->tree->setModel(_linFilteredModel);
        ui->cbAutoScroll->setEnabled(true);
        ui->tree->sortByColumn(BaseTraceViewModel::column_index, Qt::AscendingOrder);
    }
    else
    {
        ui->tree->setSortingEnabled(true);
        ui->tree->setModel(_aggFilteredModel);
        ui->cbAutoScroll->setEnabled(false);
        ui->tree->sortByColumn(BaseTraceViewModel::column_canid, Qt::AscendingOrder);
    }

    ui->tree->scrollToBottom();

    if (isChanged)
    {
        // qDebug() << "  setMode: syncing aliases and colors between models";

        _aggregatedTraceViewModel->restoreIdAliases(
            _linearTraceViewModel->getAllIdAliases());
        _linearTraceViewModel->restoreIdAliases(
            _aggregatedTraceViewModel->getAllIdAliases());

        _aggregatedTraceViewModel->restoreIdColors(
            _linearTraceViewModel->getAllIdColors());
        _linearTraceViewModel->restoreIdColors(
            _aggregatedTraceViewModel->getAllIdColors());

        // qDebug() << "  [After sync] Linear aliases:"
        //          << _linearTraceViewModel->getAllIdAliases();
        // qDebug() << "  [After sync] Aggregated aliases:"
        //          << _aggregatedTraceViewModel->getAllIdAliases();

        emit _linearTraceViewModel->dataChanged(QModelIndex(), QModelIndex(),
                                                {Qt::DisplayRole, Qt::ForegroundRole, Qt::BackgroundRole});

        emit _aggregatedTraceViewModel->dataChanged(QModelIndex(), QModelIndex(),
                                                    {Qt::DisplayRole, Qt::ForegroundRole, Qt::BackgroundRole});

        ui->cbAggregated->setChecked(_mode == mode_aggregated);

        emit(settingsChanged(this));
    }
}

void TraceWindow::setAutoScroll(bool doAutoScroll)
{
    if (doAutoScroll != _doAutoScroll)
    {
        _doAutoScroll = doAutoScroll;
        ui->cbAutoScroll->setChecked(_doAutoScroll);
        emit(settingsChanged(this));
    }
}

void TraceWindow::setTimestampMode(int mode)
{
    timestamp_mode_t new_mode;
    if ((mode >= 0) && (mode < timestamp_modes_count))
    {
        new_mode = (timestamp_mode_t)mode;
    }
    else
    {
        new_mode = timestamp_mode_absolute;
    }

    _aggregatedTraceViewModel->setTimestampMode(new_mode);
    _linearTraceViewModel->setTimestampMode(new_mode);

    if (new_mode != _timestampMode)
    {
        _timestampMode = new_mode;
        for (int i = 0; i < ui->cbTimestampMode->count(); i++)
        {
            if (ui->cbTimestampMode->itemData(i).toInt() == new_mode)
            {
                ui->cbTimestampMode->setCurrentIndex(i);
            }
        }
        emit(settingsChanged(this));
    }
}

bool TraceWindow::saveXML(Backend &backend, QDomDocument &xml, QDomElement &root)
{
    if (!ConfigurableWidget::saveXML(backend, xml, root))
        return false;

    root.setAttribute("type", "TraceWindow");
    root.setAttribute("mode", (_mode == mode_linear) ? "linear" : "aggregated");
    root.setAttribute("TimestampMode", _timestampMode);

    QDomElement elLinear = xml.createElement("LinearTraceView");
    elLinear.setAttribute("AutoScroll", ui->cbAutoScroll->isChecked() ? 1 : 0);

    BaseTraceViewModel *model = _linearTraceViewModel;

    {
        QDomElement colorsEl = xml.createElement("Colors");
        auto colors = model->getAllIdColors();

        for (auto it = colors.begin(); it != colors.end(); ++it)
        {
            QDomElement cEl = xml.createElement("Color");
            cEl.setAttribute("id", it.key());
            cEl.setAttribute("value", it.value().name(QColor::HexArgb));
            colorsEl.appendChild(cEl);
        }
        elLinear.appendChild(colorsEl);
    }

    {
        QDomElement alEl = xml.createElement("Aliases");
        auto aliases = model->getAllIdAliases();

        for (auto it = aliases.begin(); it != aliases.end(); ++it)
        {
            QDomElement aEl = xml.createElement("Alias");
            aEl.setAttribute("id", it.key());
            aEl.setAttribute("value", it.value());
            alEl.appendChild(aEl);
        }
        elLinear.appendChild(alEl);
    }

    {
        QDomElement cmEl = xml.createElement("Comments");
        auto comments = model->getAllMessageComments();

        for (auto it = comments.begin(); it != comments.end(); ++it)
        {
            QDomElement cEl = xml.createElement("Comment");
            cEl.setAttribute("msgId", it.key());
            cEl.setAttribute("value", it.value());
            cmEl.appendChild(cEl);
        }
        elLinear.appendChild(cmEl);
    }

    root.appendChild(elLinear);

    QDomElement elAggregated = xml.createElement("AggregatedTraceView");
    elAggregated.setAttribute("SortColumn", _aggregatedProxyModel->sortColumn());
    root.appendChild(elAggregated);

    return true;
}
bool TraceWindow::loadXML(Backend &backend, QDomElement &el)
{
    if (!ConfigurableWidget::loadXML(backend, el))
        return false;

    setMode((el.attribute("mode", "linear") == "linear") ? mode_linear : mode_aggregated);
    setTimestampMode(el.attribute("TimestampMode", "0").toInt());

    QDomElement elLinear = el.firstChildElement("LinearTraceView");
    setAutoScroll(elLinear.attribute("AutoScroll", "0").toInt() != 0);

    BaseTraceViewModel *model = _linearTraceViewModel;

    {
        QHash<QString, QColor> colors;
        QDomElement colorsEl = elLinear.firstChildElement("Colors");
        QDomElement cEl = colorsEl.firstChildElement("Color");

        while (!cEl.isNull())
        {
            QString id = cEl.attribute("id");
            QColor col(cEl.attribute("value"));
            if (col.isValid())
                colors[id] = col;
            cEl = cEl.nextSiblingElement("Color");
        }

        model->restoreIdColors(colors);
    }

    {
        QHash<QString, QString> aliases;
        QDomElement alEl = elLinear.firstChildElement("Aliases");
        QDomElement aEl = alEl.firstChildElement("Alias");

        while (!aEl.isNull())
        {
            QString id = aEl.attribute("id");
            QString val = aEl.attribute("value");
            aliases[id] = val;
            aEl = aEl.nextSiblingElement("Alias");
        }

        model->restoreIdAliases(aliases);
    }
    {
        QHash<int, QString> comments;
        QDomElement cmEl = elLinear.firstChildElement("Comments");
        QDomElement cEl = cmEl.firstChildElement("Comment");

        while (!cEl.isNull())
        {
            int msgId = cEl.attribute("msgId").toInt();
            QString val = cEl.attribute("value");
            comments[msgId] = val;
            cEl = cEl.nextSiblingElement("Comment");
        }

        model->restoreMessageComments(comments);
    }
    QDomElement elAggregated = el.firstChildElement("AggregatedTraceView");
    int sortColumn = elAggregated.attribute("SortColumn", "-1").toInt();
    ui->tree->sortByColumn(sortColumn, Qt::AscendingOrder);

    return true;
}

void TraceWindow::rowsInserted(const QModelIndex &parent, int first, int last)
{
    (void)parent;
    (void)first;
    (void)last;

    if (_backend->getTrace()->size() > 1000000)
    {
        _backend->clearTrace();
    }

    if ((_mode == mode_linear) && (ui->cbAutoScroll->checkState() == Qt::Checked))
    {
        ui->tree->scrollToBottom();
    }
}

void TraceWindow::on_cbAggregated_stateChanged(int i)
{
    setMode((i == Qt::Checked) ? mode_aggregated : mode_linear);
}

void TraceWindow::on_cbAutoScroll_stateChanged(int i)
{
    setAutoScroll(i == Qt::Checked);
}

void TraceWindow::on_cbTimestampMode_currentIndexChanged(int index)
{
    setTimestampMode((timestamp_mode_t)ui->cbTimestampMode->itemData(index).toInt());
}

void TraceWindow::on_cbFilterChanged()
{
    _aggFilteredModel->setFilterText(ui->filterLineEdit->text());
    _linFilteredModel->setFilterText(ui->filterLineEdit->text());
    _aggFilteredModel->invalidate();
    _linFilteredModel->invalidate();
}

void TraceWindow::on_cbTraceClearpushButton()
{
    _backend->clearTrace();
    _backend->clearLog();
}

void TraceWindow::onTraceRowDoubleClicked(const QModelIndex &index)
{
    QAbstractItemModel *viewModel = ui->tree->model();
    if (!viewModel)
        return;


    if (index.parent().isValid())
        return;

    // qDebug() << "onTraceRowDoubleClicked: mode="
    //          << (_mode == mode_linear ? "linear" : "aggregated")
    //          << "viewModel=" << viewModel
    //          << "row=" << index.row()
    //          << "col=" << index.column();

    QString idString;
    const CanMessage *exampleMsg = nullptr;

    if (_mode == mode_linear)
    {
        QModelIndex src1 = _linFilteredModel->mapToSource(index);
        QModelIndex src2 = _linearProxyModel->mapToSource(src1);

        quintptr internal = src2.internalId();
        int msgId = (internal & ~0x80000000u) - 1;

        // qDebug() << "  [linear] internalId=" << Qt::hex << internal
        //          << "msgId=" << msgId;

        if (msgId >= 0 && msgId < static_cast<int>(_backend->getTrace()->size()))
        {
            exampleMsg = _backend->getTrace()->getMessage(msgId);
            if (exampleMsg)
            {
                idString = exampleMsg->getIdString();
                // qDebug() << "  [linear] exampleMsg rawId=" << Qt::hex << exampleMsg->getRawId()
                //          << "idString=" << idString;
            }
        }
    }
    else
    {
        QModelIndex src1 = _aggFilteredModel->mapToSource(index);
        QModelIndex src2 = _aggregatedProxyModel->mapToSource(src1);

        // qDebug() << "  [aggregated] src1 row=" << src1.row()
        //          << "col=" << src1.column()
        //          << "internalPointer=" << src1.internalPointer();

        // qDebug() << "  [aggregated] src2 row=" << src2.row()
        //          << "col=" << src2.column()
        //          << "internalPointer=" << src2.internalPointer();

        AggregatedTraceViewItem *item =
            static_cast<AggregatedTraceViewItem *>(src2.internalPointer());
        if (item)
        {
            exampleMsg = &item->_lastmsg;
            idString = exampleMsg->getIdString();

            // qDebug() << "  [aggregated] exampleMsg rawId=" << Qt::hex << exampleMsg->getRawId()
            //          << "idString=" << idString;
        }
        else
        {
            // qDebug() << "  [aggregated] item is null after mapping, aborting";
        }
    }

    if (idString.isEmpty() || !exampleMsg)
    {
        // qDebug() << "  [onTraceRowDoubleClicked] No valid message found, abort.";
        return;
    }

    bool isLinear = (_mode == mode_linear);
    BaseTraceViewModel *model =
        isLinear ? static_cast<BaseTraceViewModel *>(_linearTraceViewModel)
                 : static_cast<BaseTraceViewModel *>(_aggregatedTraceViewModel);

    if (!model)
        return;

    QColor currentColor = model->messageColorForIdString(idString);
    if (!currentColor.isValid())
        currentColor = QColor(208, 208, 208);

    // qDebug() << "  isLinear=" << isLinear
    //          << "using model=" << model
    //          << "currentColor initial =" << currentColor;

    QString currentName;

    if (exampleMsg)
    {
        if (CanDbMessage *dbmsg = _backend->findDbMessage(*exampleMsg))
            currentName = dbmsg->getName();
    }

    QString currentComment;
    int msgId = -1;

    if (isLinear)
    {
        QModelIndex sourceIndex =
            _linearProxyModel->mapToSource(_linFilteredModel->mapToSource(index));

        quintptr internal = sourceIndex.internalId();
        msgId = (internal & ~0x80000000u) - 1;

        // qDebug() << "  [linear] sourceIndex internalId=" << Qt::hex << internal
        //          << "msgId=" << msgId;

        if (msgId >= 0)
            currentComment = model->commentForMessage(msgId);
    }

    QDialog dialog(this);
    dialog.setWindowTitle(isLinear
                              ? tr("Edit alias, color and comment")
                              : tr("Choose alias and color"));

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(currentName);
    form->addRow(tr("Alias:"), nameEdit);

    QWidget *colorWidget = new QWidget(&dialog);
    QHBoxLayout *colorLayout = new QHBoxLayout(colorWidget);
    colorLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton *colorButton = new QPushButton(tr("Choose color"), colorWidget);
    QFrame *colorPreview = new QFrame(colorWidget);
    colorPreview->setFrameShape(QFrame::Box);
    colorPreview->setMinimumSize(40, 20);
    colorPreview->setAutoFillBackground(true);

    auto updatePreview = [&](const QColor &c)
    {
        QPalette pal = colorPreview->palette();
        pal.setColor(QPalette::Window, c);
        colorPreview->setPalette(pal);
    };
    updatePreview(currentColor);

    colorLayout->addWidget(colorButton);
    colorLayout->addWidget(colorPreview);
    form->addRow(tr("Color:"), colorWidget);

    QObject::connect(colorButton, &QPushButton::clicked, [&]()
                     {
        QColor chosen = QColorDialog::getColor(
            currentColor,
            &dialog,
            tr("Select color for %1").arg(idString));

        if (chosen.isValid()) {
            QColor pastel = toPastel(chosen);
            // qDebug() << "  Color chosen =" << chosen
            //          << "pastel =" << pastel;
            currentColor = pastel;
            updatePreview(currentColor);
        } });

    QLineEdit *commentEdit = nullptr;
    if (isLinear)
    {
        commentEdit = new QLineEdit(&dialog);
        commentEdit->setText(currentComment);
        form->addRow(tr("Comment:"), commentEdit);
    }

    QDialogButtonBox *buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                             Qt::Horizontal, &dialog);
    form->addRow(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    int result = dialog.exec();
    // qDebug() << "  dialog result =" << result;

    if (result == QDialog::Accepted)
    {
        QString newName = nameEdit->text().trimmed();
        // qDebug() << "  newName =" << newName
        //          << "currentColor final =" << currentColor;

        if (!newName.isEmpty())
        {
            // qDebug() << "  Updating alias in BOTH models for id =" << idString;
            _linearTraceViewModel->updateAliasForIdString(idString, newName);
            _aggregatedTraceViewModel->updateAliasForIdString(idString, newName);

            // qDebug() << "  After update: Linear aliases ="
            //          << _linearTraceViewModel->getAllIdAliases();
            // qDebug() << "  After update: Aggregated aliases ="
            //          << _aggregatedTraceViewModel->getAllIdAliases();
        }

        if (currentColor.isValid())
        {
            _linearTraceViewModel->setMessageColorForIdString(idString, currentColor);
            _aggregatedTraceViewModel->setMessageColorForIdString(idString, currentColor);
        }

        if (isLinear && commentEdit && msgId >= 0)
        {
            QString newComment = commentEdit->text().trimmed();
            model->setCommentForMessage(msgId, newComment);

            QModelIndex first =
                index.sibling(index.row(), BaseTraceViewModel::column_comment);
            QModelIndex last =
                index.sibling(index.row(), BaseTraceViewModel::column_comment);
            emit model->dataChanged(first, last);
        }
    }
}

void TraceWindow::retranslateUi() {
    ui->retranslateUi(this);
    ui->cbTimestampMode->setItemText(0, tr("Absolute"));
    ui->cbTimestampMode->setItemText(1, tr("Relative"));
    ui->cbTimestampMode->setItemText(2, tr("Delta"));
}
