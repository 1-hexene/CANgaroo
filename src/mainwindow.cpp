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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtWidgets>
#include <QMdiArea>
#include <QSignalMapper>
#include <QCloseEvent>
#include <QDomDocument>
#include <QPalette>

#include <core/MeasurementSetup.h>
#include <core/CanTrace.h>
#include <window/TraceWindow/TraceWindow.h>
#include <window/SetupDialog/SetupDialog.h>
#include <window/LogWindow/LogWindow.h>
#include <window/GraphWindow/GraphWindow.h>
#include <window/CanStatusWindow/CanStatusWindow.h>
#include <window/RawTxWindow/RawTxWindow.h>
#include <window/TxGeneratorWindow/TxGeneratorWindow.h>

#include <driver/SLCANDriver/SLCANDriver.h>
#include <driver/GrIPDriver/GrIPDriver.h>
#include <driver/CANBlastDriver/CANBlasterDriver.h>
#include <window/TraceWindow/LinearTraceViewModel.h>
#include "window/TraceWindow/AggregatedTraceViewModel.h"

#if defined(__linux__)
#include <driver/SocketCanDriver/SocketCanDriver.h>
#else
#include <driver/CandleApiDriver/CandleApiDriver.h>
#endif
#include <QActionGroup>
#include <QEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _baseWindowTitle = windowTitle();

    QIcon icon(":/assets/cangaroo.png");
    setWindowIcon(icon);

    connect(ui->action_Trace_View, SIGNAL(triggered()), this, SLOT(createTraceWindow()));
    connect(ui->actionLog_View, SIGNAL(triggered()), this, SLOT(addLogWidget()));
    connect(ui->actionGraph_View, SIGNAL(triggered()), this, SLOT(createGraphWindow()));
    connect(ui->actionGraph_View_2, SIGNAL(triggered()), this, SLOT(addGraphWidget()));
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(showSetupDialog()));
    connect(ui->actionTransmit_View, SIGNAL(triggered()), this, SLOT(addRawTxWidget()));

    connect(ui->actionStart_Measurement, SIGNAL(triggered()), this, SLOT(startMeasurement()));
    connect(ui->actionStop_Measurement, SIGNAL(triggered()), this, SLOT(stopMeasurement()));

    connect(&backend(), SIGNAL(beginMeasurement()), this, SLOT(updateMeasurementActions()));
    connect(&backend(), SIGNAL(endMeasurement()), this, SLOT(updateMeasurementActions()));
    updateMeasurementActions();

    connect(ui->actionSave_Trace_to_file, SIGNAL(triggered(bool)), this, SLOT(saveTraceToFile()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
    QMenu *traceMenu = ui->menu_Trace;

    QAction *actionExportFull = new QAction(tr("Export full trace"), this);
    connect(actionExportFull, &QAction::triggered, this, &MainWindow::exportFullTrace);
    traceMenu->addAction(actionExportFull);
    QAction *actionImportFull = new QAction(tr("Import full trace"), this);
    connect(actionImportFull, &QAction::triggered, this, &MainWindow::importFullTrace);
    traceMenu->addAction(actionImportFull);

#if defined(__linux__)
    Backend::instance().addCanDriver(*(new SocketCanDriver(Backend::instance())));
#else
    //Backend::instance().addCanDriver(*(new CandleApiDriver(Backend::instance())));
#endif
    Backend::instance().addCanDriver(*(new SLCANDriver(Backend::instance())));
    Backend::instance().addCanDriver(*(new GrIPDriver(Backend::instance())));
    // Backend::instance().addCanDriver(*(new CANBlasterDriver(Backend::instance())));

    setWorkspaceModified(false);
    newWorkspace();

    // NOTE: must be called after drivers/plugins are initialized
    _setupDlg = new SetupDialog(Backend::instance(), 0);

    _showSetupDialog_first = false;
    qApp->installTranslator(&m_translator);
    createLanguageMenu();
    if (!m_languageActionGroup->actions().isEmpty())
    {
        m_languageActionGroup->actions().first()->trigger();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateMeasurementActions()
{
    bool running = backend().isMeasurementRunning();
    ui->actionStart_Measurement->setEnabled(!running);
    ui->actionSetup->setEnabled(!running);
    ui->actionStop_Measurement->setEnabled(running);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (askSaveBecauseWorkspaceModified() != QMessageBox::Cancel)
    {
        backend().stopMeasurement();
        event->accept();
    }
    else
    {
        event->ignore();
    }

    /*QSettings settings("MyCompany", "MyApp");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);*/
}

/*void MainWindow::readSettings()
{
    QSettings settings("MyCompany", "MyApp");
    restoreGeometry(settings.value("myWidget/geometry").toByteArray());
    restoreState(settings.value("myWidget/windowState").toByteArray());
}*/

Backend &MainWindow::backend()
{
    return Backend::instance();
}

QMainWindow *MainWindow::createTab(QString title)
{
    QMainWindow *mm = new QMainWindow(this);
    QPalette pal(palette());
    pal.setColor(QPalette::Window, QColor(0xeb, 0xeb, 0xeb));
    mm->setAutoFillBackground(true);
    mm->setPalette(pal);
    ui->mainTabs->addTab(mm, title);
    return mm;
}

QMainWindow *MainWindow::currentTab()
{
    return (QMainWindow *)ui->mainTabs->currentWidget();
}

void MainWindow::stopAndClearMeasurement()
{
    backend().stopMeasurement();
    QCoreApplication::processEvents();
    backend().clearTrace();
    backend().clearLog();
}

void MainWindow::clearWorkspace()
{
    ui->mainTabs->clear();
    _workspaceFileName.clear();
    setWorkspaceModified(false);
}

bool MainWindow::loadWorkspaceTab(QDomElement el)
{
    QMainWindow *mw = 0;
    QString type = el.attribute("type");
    if (type == "TraceWindow")
    {
        mw = createTraceWindow(el.attribute("title"));
    }
    else if (type == "GraphWindow")
    {
        mw = createGraphWindow(el.attribute("title"));
    }
    else
    {
        return false;
    }

    if (mw)
    {
        ConfigurableWidget *mdi = dynamic_cast<ConfigurableWidget *>(mw->centralWidget());
        if (mdi)
        {
            mdi->loadXML(backend(), el);
        }
    }

    return true;
}

bool MainWindow::loadWorkspaceSetup(QDomElement el)
{
    MeasurementSetup setup(&backend());
    if (setup.loadXML(backend(), el))
    {
        backend().setSetup(setup);
        return true;
    }
    else
    {
        return false;
    }
}

void MainWindow::loadWorkspaceFromFile(QString filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        log_error(QString("Cannot open workspace settings file: %1").arg(filename));
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        file.close();
        log_error(QString("Cannot load settings from file: %1").arg(filename));
        return;
    }
    file.close();

    stopAndClearMeasurement();
    clearWorkspace();

    QDomElement tabsRoot = doc.firstChild().firstChildElement("tabs");
    QDomNodeList tabs = tabsRoot.elementsByTagName("tab");
    for (int i = 0; i < tabs.length(); i++)
    {
        if (!loadWorkspaceTab(tabs.item(i).toElement()))
        {
            log_warning(QString("Could not read window %1 from file: %2").arg(QString::number(i), filename));
            continue;
        }
    }

    QDomElement setupRoot = doc.firstChild().firstChildElement("setup");
    if (loadWorkspaceSetup(setupRoot))
    {
        _workspaceFileName = filename;
        setWorkspaceModified(false);
    }
    else
    {
        log_error(QString("Unable to read measurement setup from workspace config file: %1").arg(filename));
    }
}

bool MainWindow::saveWorkspaceToFile(QString filename)
{
    QDomDocument doc;
    QDomElement root = doc.createElement("cangaroo-workspace");
    doc.appendChild(root);

    QDomElement tabsRoot = doc.createElement("tabs");
    root.appendChild(tabsRoot);

    for (int i = 0; i < ui->mainTabs->count(); i++)
    {
        QMainWindow *w = (QMainWindow *)ui->mainTabs->widget(i);

        QDomElement tabEl = doc.createElement("tab");
        tabEl.setAttribute("title", ui->mainTabs->tabText(i));

        ConfigurableWidget *mdi = dynamic_cast<ConfigurableWidget *>(w->centralWidget());
        if (!mdi->saveXML(backend(), doc, tabEl))
        {
            log_error(QString("Cannot save window settings to file: %1").arg(filename));
            return false;
        }

        tabsRoot.appendChild(tabEl);
    }

    QDomElement setupRoot = doc.createElement("setup");
    if (!backend().getSetup().saveXML(backend(), doc, setupRoot))
    {
        log_error(QString("Cannot save measurement setup to file: %1").arg(filename));
        return false;
    }
    root.appendChild(setupRoot);

    QFile outFile(filename);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&outFile);
        stream << doc.toString();
        outFile.close();
        _workspaceFileName = filename;
        setWorkspaceModified(false);
        log_info(QString("Saved workspace settings to file: %1").arg(filename));
        return true;
    }
    else
    {
        log_error(QString("Cannot open workspace file for writing: %1").arg(filename));
        return false;
    }
}

void MainWindow::newWorkspace()
{
    if (askSaveBecauseWorkspaceModified() != QMessageBox::Cancel)
    {
        stopAndClearMeasurement();
        clearWorkspace();
        createTraceWindow();
        backend().setDefaultSetup();
    }
}

void MainWindow::loadWorkspace()
{
    if (askSaveBecauseWorkspaceModified() != QMessageBox::Cancel)
    {
        QString filename = QFileDialog::getOpenFileName(this, tr("Open workspace configuration"), "", tr("Workspace config files (*.cangaroo)"));
        if (!filename.isNull())
        {
            loadWorkspaceFromFile(filename);
        }
    }
}

bool MainWindow::saveWorkspace()
{
    if (_workspaceFileName.isEmpty())
    {
        return saveWorkspaceAs();
    }
    else
    {
        return saveWorkspaceToFile(_workspaceFileName);
    }
}

bool MainWindow::saveWorkspaceAs()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save workspace configuration"), "", tr("Workspace config files (*.cangaroo)"));
    if (!filename.isNull())
    {
        return saveWorkspaceToFile(filename);
    }
    else
    {
        return false;
    }
}

void MainWindow::setWorkspaceModified(bool modified)
{
    _workspaceModified = modified;

    QString title = _baseWindowTitle;
    if (!_workspaceFileName.isEmpty())
    {
        QFileInfo fi(_workspaceFileName);
        title += " - " + fi.fileName();
    }
    if (_workspaceModified)
    {
        title += '*';
    }
    setWindowTitle(title);
}

int MainWindow::askSaveBecauseWorkspaceModified()
{
    if (_workspaceModified)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("The current workspace has been modified."));
        msgBox.setInformativeText(tr("Do you want to save your changes?"));
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        /*msgBox.setButtonText(QMessageBox::Save, QString(tr("Save")));
        msgBox.setButtonText(QMessageBox::Discard, QString(tr("Discard")));
        msgBox.setButtonText(QMessageBox::Cancel, QString(tr("Cancel")));*/

        int result = msgBox.exec();
        if (result == QMessageBox::Save)
        {
            if (saveWorkspace())
            {
                return QMessageBox::Save;
            }
            else
            {
                return QMessageBox::Cancel;
            }
        }
        return result;
    }
    else
    {
        return QMessageBox::Discard;
    }
}

QMainWindow *MainWindow::createTraceWindow(QString title)
{
    if (title.isNull())
    {
        title = tr("Trace");
    }
    QMainWindow *mm = createTab(title);
    mm->setCentralWidget(new TraceWindow(mm, backend()));

    QDockWidget *dockLogWidget = addLogWidget(mm);
    QDockWidget *dockStatusWidget = addStatusWidget(mm);
    QDockWidget *dockRawTxWidget = addRawTxWidget(mm);
    QDockWidget *dockGeneratorWidget = addTxGeneratorWidget(mm);

    mm->splitDockWidget(dockRawTxWidget, dockLogWidget, Qt::Horizontal);
    mm->splitDockWidget(dockGeneratorWidget, dockLogWidget, Qt::Horizontal);
    mm->tabifyDockWidget(dockGeneratorWidget, dockRawTxWidget);
    mm->splitDockWidget(dockStatusWidget, dockLogWidget, Qt::Horizontal);
    mm->tabifyDockWidget(dockStatusWidget, dockLogWidget);
    ui->mainTabs->setCurrentWidget(mm);
    return mm;
}

QMainWindow *MainWindow::createGraphWindow(QString title)
{
    if (title.isNull())
    {
        title = tr("Graph");
    }
    QMainWindow *mm = createTab(title);
    mm->setCentralWidget(new GraphWindow(mm, backend()));
    addLogWidget(mm);

    return mm;
}

void MainWindow::addGraphWidget(QMainWindow *parent)
{
    if (!parent)
    {
        parent = currentTab();
    }
    QDockWidget *dock = new QDockWidget(tr("Graph"), parent);
    dock->setWidget(new GraphWindow(dock, backend()));
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
}

QDockWidget *MainWindow::addRawTxWidget(QMainWindow *parent)
{
    if (!parent)
    {
        parent = currentTab();
    }
    QDockWidget *dock = new QDockWidget(tr("Transmit View"), parent);
    dock->setWidget(new RawTxWindow(dock, backend()));
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
    return dock;
}

QDockWidget *MainWindow::addLogWidget(QMainWindow *parent)
{
    if (!parent)
    {
        parent = currentTab();
    }
    QDockWidget *dock = new QDockWidget(tr("Log"), parent);
    dock->setWidget(new LogWindow(dock, backend()));
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
    return dock;
}

QDockWidget *MainWindow::addStatusWidget(QMainWindow *parent)
{
    if (!parent)
    {
        parent = currentTab();
    }
    QDockWidget *dock = new QDockWidget(tr("CAN Status"), parent);
    dock->setWidget(new CanStatusWindow(dock, backend()));
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
    return dock;
}

QDockWidget *MainWindow::addTxGeneratorWidget(QMainWindow *parent)
{
    if (!parent)
    {
        parent = currentTab();
    }
    QDockWidget *dock = new QDockWidget(tr("Generator View"), parent);
    dock->setWidget(new TxGeneratorWindow(dock, backend()));
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);
    return dock;
}

void MainWindow::on_actionCan_Status_View_triggered()
{
    addStatusWidget();
}

bool MainWindow::showSetupDialog()
{
    MeasurementSetup new_setup(&backend());
    new_setup.cloneFrom(backend().getSetup());
    backend().setDefaultSetup();
    if (backend().getSetup().countNetworks() == new_setup.countNetworks())
    {
        backend().setSetup(new_setup);
    }
    else
    {
        new_setup.cloneFrom(backend().getSetup());
    }
    if (_setupDlg->showSetupDialog(new_setup))
    {
        if (!_setupDlg->isReflashNetworks())
            backend().setSetup(new_setup);

        setWorkspaceModified(true);
        _showSetupDialog_first = true;
        return true;
    }
    else
    {
        return false;
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(this,
                       tr("About CANgaroo"),
                       "CANgaroo\n"
                       "Open Source CAN bus analyzer\n"
                       "\n"
                       "version 0.3.1\n"
                       "\n"
                       "(c)2015-2017 Hubert Denkmair\n"
                       "(c)2018-2022 Ethan Zonca\n"
                       "(c)2024 WeAct Studio\n"
                       "(c)2024-2026 Schildkroet\n"
                       "(c)2025 Wikilift");
}

void MainWindow::startMeasurement()
{
    if (!_showSetupDialog_first)
    {
        if (showSetupDialog())
        {
            backend().clearTrace();
            backend().startMeasurement();
            _showSetupDialog_first = true;
        }
    }
    else
    {
        backend().startMeasurement();
    }
}

void MainWindow::stopMeasurement()
{
    backend().stopMeasurement();
}

void MainWindow::saveTraceToFile()
{
    QString filters("Vector ASC (*.asc);;Linux candump (*.candump))");
    QString defaultFilter("Vector ASC (*.asc)");

    QFileDialog fileDialog(0, "Save Trace to file", QDir::currentPath(), filters);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setOption(QFileDialog::DontConfirmOverwrite, false);
    // fileDialog.setConfirmOverwrite(true);
    fileDialog.selectNameFilter(defaultFilter);
    fileDialog.setDefaultSuffix("asc");
    if (fileDialog.exec())
    {
        QString filename = fileDialog.selectedFiles()[0];
        QFile file(filename);
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        {
            if (filename.endsWith(".candump", Qt::CaseInsensitive))
            {
                backend().getTrace()->saveCanDump(file);
            }
            else
            {
                backend().getTrace()->saveVectorAsc(file);
            }

            file.close();
        }
        else
        {
            // TODO error message
        }
    }
}

void MainWindow::on_action_TraceClear_triggered()
{
    backend().clearTrace();
    backend().clearLog();
}

void MainWindow::on_action_WorkspaceSave_triggered()
{
    saveWorkspace();
}

void MainWindow::on_action_WorkspaceSaveAs_triggered()
{
    saveWorkspaceAs();
}

void MainWindow::on_action_WorkspaceOpen_triggered()
{
    loadWorkspace();
}

void MainWindow::on_action_WorkspaceNew_triggered()
{
    newWorkspace();
}

void MainWindow::on_actionGenerator_View_triggered()
{
    addTxGeneratorWidget();
}

void MainWindow::switchLanguage(QAction *action)
{
    QString locale = action->data().toString();

    qApp->removeTranslator(&m_translator);

    if (locale == "en_US")
    {
        std::ignore = m_translator.load("");
    }
    else
    {
        QString qmPath = ":/translations/i18n_" + locale + ".qm";
        if (!m_translator.load(qmPath))
        {
            // todo: launch error
            qDebug() << "Could not load translation: " << qmPath;
        }
    }

    qApp->installTranslator(&m_translator);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {

        ui->retranslateUi(this);

        _baseWindowTitle = tr("CANgaroo");
        setWorkspaceModified(_workspaceModified);

        m_languageMenu->setTitle(tr("&Language"));
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::createLanguageMenu()
{
    m_languageMenu = ui->menuHelp->addMenu(tr("&Language"));

    m_languageActionGroup = new QActionGroup(this);

    connect(m_languageActionGroup, &QActionGroup::triggered, this, &MainWindow::switchLanguage);

    QAction *actionEn = new QAction(tr("English"), this);
    actionEn->setCheckable(true);
    actionEn->setChecked(true);
    actionEn->setData("en_US");
    m_languageMenu->addAction(actionEn);
    m_languageActionGroup->addAction(actionEn);

    QAction *actionEs = new QAction(tr("Español"), this);
    actionEs->setCheckable(true);
    actionEs->setData("es_ES");
    m_languageMenu->addAction(actionEs);
    m_languageActionGroup->addAction(actionEs);

    QAction *actionDe = new QAction(tr("Deutsch"), this);
    actionDe->setCheckable(true);
    actionDe->setData("de_DE");
    m_languageMenu->addAction(actionDe);
    m_languageActionGroup->addAction(actionDe);

    QAction *actionCN = new QAction(tr("Chinese"), this);
    actionCN->setCheckable(true);
    actionCN->setData("zh_cn");
    m_languageMenu->addAction(actionCN);
    m_languageActionGroup->addAction(actionCN);
}

void MainWindow::exportFullTrace()
{
    TraceWindow *tw = currentTab()->findChild<TraceWindow *>();
    if (!tw)
    {
        QMessageBox::warning(this, tr("Error"), tr("No Trace window active"));
        return;
    }

    auto *model = tw->linearModel();
    CanTrace *trace = backend().getTrace();

    QString filename = QFileDialog::getSaveFileName(
        this,
        tr("Export full trace"),
        "",
        tr("CANgaroo Trace (*.ctrace)"));
    if (filename.isEmpty())
        return;
    if (!filename.endsWith(".ctrace"))
        filename += ".ctrace";

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot write file"));
        return;
    }

    QJsonObject root;

    QJsonArray msgsJson;
    unsigned long count = trace->size();

    for (unsigned long i = 0; i < count; i++)
    {
        const CanMessage *msg = trace->getMessage(i);
        if (!msg)
            continue;

        QJsonObject m;
        m["timestamp"] = msg->getFloatTimestamp();
        m["raw_id"] = (int)msg->getRawId();
        m["id"] = msg->getIdString();
        m["dlc"] = msg->getLength();
        m["data"] = msg->getDataHexString();
        m["direction"] = msg->isRX() ? "RX" : "TX";
        m["comment"] = model->exportedComment(i);

        msgsJson.append(m);
    }

    root["messages"] = msgsJson;

    QJsonObject colorsJson;
    for (auto it = model->exportedColors().begin(); it != model->exportedColors().end(); ++it)
        colorsJson[it.key()] = it.value().name();
    root["colors"] = colorsJson;

    QJsonObject aliasJson;
    for (auto it = model->exportedAliases().begin(); it != model->exportedAliases().end(); ++it)
        aliasJson[it.key()] = it.value();
    root["aliases"] = aliasJson;

    file.write(QJsonDocument(root).toJson());
    file.close();
}



void MainWindow::importFullTrace()
{
    TraceWindow *tw = currentTab()->findChild<TraceWindow *>();
    if (!tw)
    {
        QMessageBox::warning(this, tr("Error"), tr("No Trace window active"));
        return;
    }

    auto *linear = tw->linearModel();
    auto *agg    = tw->aggregatedModel();

    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("Import full trace"),
        "",
        tr("CANgaroo Trace (*.ctrace)")
    );
    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot read file"));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject root = doc.object();

    backend().clearTrace();


    {
        QJsonObject colors = root["colors"].toObject();
        for (auto it = colors.begin(); it != colors.end(); ++it)
        {
            QColor c(it.value().toString());

            linear->setMessageColorForIdString(it.key(), c);
            agg->setMessageColorForIdString(it.key(), c);
        }
    }

 
    {
        QJsonObject aliases = root["aliases"].toObject();
        for (auto it = aliases.begin(); it != aliases.end(); ++it)
        {
            QString alias = it.value().toString();

            linear->updateAliasForIdString(it.key(), alias);
            agg->updateAliasForIdString(it.key(), alias);
        }
    }


    QJsonArray msgs = root["messages"].toArray();

    for (int i = 0; i < msgs.size(); i++)
    {
        QJsonObject m = msgs[i].toObject();
        CanMessage msg;

        double ts = m["timestamp"].toDouble();
        struct timeval tv;
        tv.tv_sec  = (time_t)ts;
        tv.tv_usec = (ts - tv.tv_sec) * 1e6;
        msg.setTimestamp(tv);

        msg.setRawId(m["raw_id"].toInt());
        msg.setLength(m["dlc"].toInt());

        QByteArray ba = QByteArray::fromHex(m["data"].toString().toUtf8());
        for (int b = 0; b < ba.size(); b++)
            msg.setByte(b, (uint8_t)ba[b]);

        msg.setRX(m["direction"].toString() == "RX");

        backend().getTrace()->enqueueMessage(msg, false);

        QString comment = m["comment"].toString();
        if (!comment.isEmpty())
        {
            linear->setCommentForMessage(i, comment);
            agg->setCommentForMessage(i, comment);
        }
    }


    QMetaObject::invokeMethod(linear, "modelReset", Qt::DirectConnection);
    QMetaObject::invokeMethod(agg,    "modelReset", Qt::DirectConnection);

    linear->layoutChanged();
    agg->layoutChanged();
}
