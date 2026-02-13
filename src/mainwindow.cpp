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
#include <QTimer>
#include <QDomDocument>
#include <QPalette>
#include <QActionGroup>
#include <QEvent>

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

#if defined(__linux__)
#include <driver/SocketCanDriver/SocketCanDriver.h>
#else
#include <driver/CandleApiDriver/CandleApiDriver.h>
#endif



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _baseWindowTitle = windowTitle();

    QCoreApplication::setApplicationVersion(VERSION_STRING);
    QCoreApplication::setOrganizationName("Schildkroet");
    QCoreApplication::setApplicationName("CANgaroo");

    QLabel* versionLabel = new QLabel(this);
    versionLabel->setText(QString("v%1").arg(QCoreApplication::applicationVersion()));
    versionLabel->setStyleSheet("padding-right: 15px; color: #000000; font-weight: bold; font-size: 11px;");
    statusBar()->addPermanentWidget(versionLabel);

    QIcon icon(":/assets/cangaroo.png");
    setWindowIcon(icon);

    connect(ui->action_Trace_View, SIGNAL(triggered()), this, SLOT(createTraceWindow()));
    connect(ui->actionLog_View, SIGNAL(triggered()), this, SLOT(addLogWidget()));
    connect(ui->actionGraph_View, SIGNAL(triggered()), this, SLOT(createGraphWindow()));
    connect(ui->actionGraph_View_2, SIGNAL(triggered()), this, SLOT(addGraphWidget()));
    connect(ui->actionSetup, SIGNAL(triggered()), this, SLOT(showSetupDialog()));
    connect(ui->actionTransmit_View, SIGNAL(triggered()), this, SLOT(addRawTxWidget()));
    connect(ui->actionGenerator_View, SIGNAL(triggered()), this, SLOT(on_actionGenerator_View_triggered()));

    QAction *actionStandaloneGraph = new QAction(tr("Standalone Graph"), this);
    actionStandaloneGraph->setShortcut(QKeySequence("Ctrl+Shift+B"));
    ui->menuWindow->addAction(actionStandaloneGraph);
    connect(actionStandaloneGraph, &QAction::triggered, this, &MainWindow::createStandaloneGraphWindow);

    QAction *actionTheme = new QAction(tr("Theme..."), this);
    ui->menuWindow->addAction(actionTheme);
    connect(actionTheme, &QAction::triggered, this, &MainWindow::showThemeDialog);

    connect(ui->actionStart_Measurement, SIGNAL(triggered()), this, SLOT(startMeasurement()));
    connect(ui->btnStartMeasurement, SIGNAL(released()), this, SLOT(startMeasurement()));
    connect(ui->actionStop_Measurement, SIGNAL(triggered()), this, SLOT(stopMeasurement()));
    connect(ui->btnStopMeasurement, SIGNAL(released()), this, SLOT(stopMeasurement()));
    connect(ui->btnSetupMeasurement, SIGNAL(released()), this, SLOT(showSetupDialog()));

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

    // Load settings
    bool restoreEnabled = settings.value("ui/restoreWindowGeometry", false).toBool();
    bool CANblasterEnabled = settings.value("mainWindow/CANblaster", false).toBool();

    ui->actionRestore_Window->setChecked(restoreEnabled);
    ui->actionCANblaster->setChecked(CANblasterEnabled);

    if (settings.value("ui/restoreWindowGeometry", false).toBool()) {
        if (!restoreGeometry(settings.value("mainWindow/geometry").toByteArray()))
        {
            resize(1365, 900);
            move(QGuiApplication::primaryScreen()->availableGeometry().center() - rect().center());
            settings.setValue("mainWindow/maximized", false);
        }
        restoreState(settings.value("mainWindow/state").toByteArray());
    }

#if defined(__linux__)
    Backend::instance().addCanDriver(*(new SocketCanDriver(Backend::instance())));
#else
    Backend::instance().addCanDriver(*(new CandleApiDriver(Backend::instance())));
#endif
    Backend::instance().addCanDriver(*(new SLCANDriver(Backend::instance())));
    Backend::instance().addCanDriver(*(new GrIPDriver(Backend::instance())));

    if (CANblasterEnabled)
    {
        Backend::instance().addCanDriver(*(new CANBlasterDriver(Backend::instance())));
    }

    setWorkspaceModified(false);
    newWorkspace();

    // NOTE: must be called after drivers/plugins are initialized
    _setupDlg = new SetupDialog(Backend::instance(), 0);

    _showSetupDialog_first = false;

    // Start/Stop Button Design
    setStyleSheet(
        "QMainWindow::separator {"
        "  background: transparent;"
        "  width: 6px;"
        "  height: 6px;"
        "}"
        "QMainWindow::separator:hover {"
        "  background: #0078d7;"
        "}"
        "QSplitter::handle {"
        "  background: transparent;"
        "  width: 6px;"
        "  height: 6px;"
        "}"
        "QSplitter::handle:hover {"
        "  background: #0078d7;"
        "}"
        "QPushButton#btnStartMeasurement {"
        "  background-color: #28a745;"
        "  color: white;"
        "  border-radius: 12px;"
        "  padding: 5px 15px;"
        "  font-weight: bold;"
        "}"
        "QPushButton#btnStartMeasurement:hover {"
        "  background-color: #218838;"
        "}"
        "QPushButton#btnStartMeasurement:disabled {"
        "  background-color: #94d3a2;"
        "}"
        "QPushButton#btnStopMeasurement {"
        "  background-color: #dc3545;"
        "  color: white;"
        "  border-radius: 12px;"
        "  padding: 5px 15px;"
        "  font-weight: bold;"
        "}"
        "QPushButton#btnStopMeasurement:hover {"
        "  background-color: #c82333;"
        "}"
        "QPushButton#btnStopMeasurement:disabled {"
        "  background-color: #f1aeb5;"
        "}"
        );

    qApp->installTranslator(&m_translator);
    createLanguageMenu();
    if (!m_languageActionGroup->actions().isEmpty())
    {
        m_languageActionGroup->actions().first()->trigger();
    }

    // Load saved application style/theme
    QString savedStyle = settings.value("ui/applicationStyle", "").toString();
    if (!savedStyle.isEmpty()) {
        QStringList availableStyles = QStyleFactory::keys();
        bool styleFound = false;
        for (const QString &style : availableStyles) {
            if (style.compare(savedStyle, Qt::CaseInsensitive) == 0) {
                styleFound = true;
                break;
            }
        }
        if (styleFound) {
            QApplication::setStyle(QStyleFactory::create(savedStyle));
            qDebug() << "Loaded saved style:" << savedStyle;
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (askSaveBecauseWorkspaceModified() != QMessageBox::Cancel)
    {
        backend().stopMeasurement();

        // Auto-save to the current workspace file if one is set
        if (!_workspaceFileName.isEmpty())
        {
            saveWorkspaceToFile(_workspaceFileName);
        }

        event->accept();
    }
    else
    {
        event->ignore();
    }

    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/state", saveState());
    settings.setValue("mainWindow/maximized", isMaximized());
    settings.setValue("ui/restoreWindowGeometry", ui->actionRestore_Window->isChecked());
    settings.setValue("mainWindow/CANblaster", ui->actionCANblaster->isChecked());

    QMainWindow::closeEvent(event);
}

bool MainWindow::isMaxi()
{
    return settings.value("mainWindow/maximized").toBool();
}

void MainWindow::updateMeasurementActions()
{
    bool running = backend().isMeasurementRunning();
    ui->actionStart_Measurement->setEnabled(!running);
    ui->actionSetup->setEnabled(!running);
    ui->actionStop_Measurement->setEnabled(running);

    ui->btnStartMeasurement->setEnabled(!running);
    ui->btnSetupMeasurement->setEnabled(!running);
    ui->btnStopMeasurement->setEnabled(running);
}

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
    return (QMainWindow*)ui->mainTabs->currentWidget();
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
    while (ui->mainTabs->count() > 0) {
        QWidget *w = ui->mainTabs->widget(0);
        ui->mainTabs->removeTab(0);
        delete w;
    }

    // Close and clear standalone windows to prevent dangling pointers to signals
    while (!_standaloneGraphWindows.isEmpty()) {
        GraphWindow *gw = _standaloneGraphWindows.takeFirst();
        if (gw) {
            gw->close(); // This will trigger WA_DeleteOnClose
        }
    }

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
        ConfigurableWidget *mdi = dynamic_cast<ConfigurableWidget*>(mw->centralWidget());
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
        log_error(QString(tr("Cannot open workspace settings file: %1")).arg(filename));
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        file.close();
        log_error(QString(tr("Cannot load settings from file: %1")).arg(filename));
        return;
    }
    file.close();

    stopAndClearMeasurement();
    clearWorkspace();

    QDomElement root = doc.documentElement();
    if (root.tagName() != "cangaroo-workspace")
    {
        log_error(QString("Invalid workspace file format: %1").arg(filename));
        return;
    }

    QDomElement tabsRoot = root.firstChildElement("tabs");
    QDomNodeList tabs = tabsRoot.elementsByTagName("tab");
    for (int i = 0; i < tabs.length(); i++)
    {
        if (!loadWorkspaceTab(tabs.item(i).toElement()))
        {
            log_warning(QString(tr("Could not read window %1 from file: %2")).arg(QString::number(i), filename));
            continue;
        }
    }

    QDomElement setupRoot = root.firstChildElement("setup");
    if (loadWorkspaceSetup(setupRoot))
    {
        _workspaceFileName = filename;
    }
    else
    {
        log_error(QString(tr("Unable to read measurement setup from workspace config file: %1")).arg(filename));
    }

    if (ui->mainTabs->count() > 0)
    {
        ui->mainTabs->setCurrentIndex(0);
    }
    setWorkspaceModified(false);
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
        QMainWindow *w = (QMainWindow*)ui->mainTabs->widget(i);

        QDomElement tabEl = doc.createElement("tab");
        tabEl.setAttribute("title", ui->mainTabs->tabText(i));

        ConfigurableWidget *mdi = dynamic_cast<ConfigurableWidget*>(w->centralWidget());
        if (!mdi->saveXML(backend(), doc, tabEl))
        {
            log_error(QString(tr("Cannot save window settings to file: %1")).arg(filename));
            return false;
        }

        tabsRoot.appendChild(tabEl);
    }

    QDomElement setupRoot = doc.createElement("setup");
    if (!backend().getSetup().saveXML(backend(), doc, setupRoot))
    {
        log_error(QString(tr("Cannot save measurement setup to file: %1")).arg(filename));
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
        log_info(QString(tr("Saved workspace settings to file: %1")).arg(filename));
        return true;
    }
    else
    {
        log_error(QString(tr("Cannot open workspace file for writing: %1")).arg(filename));
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

        // Clear the workspace filename for a fresh start
        _workspaceFileName.clear();
        setWorkspaceModified(false);
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
        // Ensure the filename has .cangaroo extension
        if (!filename.endsWith(".cangaroo", Qt::CaseInsensitive))
        {
            filename += ".cangaroo";
        }
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
    TraceWindow *trace = new TraceWindow(mm, backend());
    mm->setCentralWidget(trace);

    QDockWidget *dockLogWidget = addLogWidget(mm);
    QDockWidget *dockStatusWidget = addStatusWidget(mm);
    QDockWidget *dockRawTxWidget = addRawTxWidget(mm);
    QDockWidget *dockGeneratorWidget = addTxGeneratorWidget(mm);
    QDockWidget *dockGraphWidget = addGraphWidget(mm);

    TxGeneratorWindow *gen = qobject_cast<TxGeneratorWindow*>(dockGeneratorWidget->widget());
    RawTxWindow *rawtx = qobject_cast<RawTxWindow*>(dockRawTxWidget->widget());
    if (gen && rawtx) {
        connect(gen, &TxGeneratorWindow::messageSelected, rawtx, &RawTxWindow::setMessage);
        connect(rawtx, &RawTxWindow::messageUpdated, gen, &TxGeneratorWindow::updateMessage);
        connect(gen, &TxGeneratorWindow::loopbackFrame, trace, &TraceWindow::addMessage);
    }

    mm->splitDockWidget(dockRawTxWidget,dockLogWidget,Qt::Horizontal);
    mm->splitDockWidget(dockGeneratorWidget,dockLogWidget,Qt::Horizontal);
    mm->splitDockWidget(dockGraphWidget,dockLogWidget,Qt::Horizontal);
    mm->tabifyDockWidget(dockGeneratorWidget, dockRawTxWidget); // Generator first, Message next
    mm->tabifyDockWidget(dockRawTxWidget, dockGraphWidget);
    mm->splitDockWidget(dockStatusWidget,dockLogWidget,Qt::Horizontal);
    mm->tabifyDockWidget(dockStatusWidget, dockLogWidget); // Status first, Log next

    // Use QTimer to resize docks and ensure correct focus/visibility after layout is complete
    QTimer::singleShot(0, mm, [mm, dockLogWidget, dockRawTxWidget, dockGeneratorWidget, dockStatusWidget]() {
        dockStatusWidget->show();
        dockStatusWidget->raise();
        dockGeneratorWidget->show();
        dockGeneratorWidget->raise();

        mm->resizeDocks({dockLogWidget, dockRawTxWidget, dockGeneratorWidget, dockStatusWidget}, {600, 600, 600, 600}, Qt::Vertical);
        mm->resizeDocks({dockLogWidget, dockRawTxWidget, dockGeneratorWidget, dockStatusWidget}, {1200, 1200, 1200, 1200}, Qt::Horizontal);
    });

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

void MainWindow::createStandaloneGraphWindow()
{
    GraphWindow *gw = new GraphWindow(nullptr, backend());
    gw->setWindowTitle(tr("Standalone Graph"));
    gw->setAttribute(Qt::WA_DeleteOnClose);

    _standaloneGraphWindows.append(gw);
    connect(gw, &QObject::destroyed, this, [this, gw]() {
        _standaloneGraphWindows.removeAll(gw);
    });

    gw->show();
}

QDockWidget *MainWindow::addGraphWidget(QMainWindow *parent)
{
    if (!parent)
    {
        parent = currentTab();
    }
    QDockWidget *dock = new QDockWidget(tr("Graph"), parent);
    dock->setWidget(new GraphWindow(dock, backend()));
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);

    return dock;
}

QDockWidget *MainWindow::addRawTxWidget(QMainWindow *parent)
{
    if (!parent)
    {
        parent = currentTab();
    }
    QDockWidget *dock = new QDockWidget(tr("Message View"), parent);
    RawTxWindow *rawTx = new RawTxWindow(dock, backend());
    dock->setWidget(rawTx);
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);

    TxGeneratorWindow *gen = parent->findChild<TxGeneratorWindow*>();
    if (gen) {
        connect(gen, &TxGeneratorWindow::messageSelected, rawTx, &RawTxWindow::setMessage);
        connect(rawTx, &RawTxWindow::messageUpdated, gen, &TxGeneratorWindow::updateMessage);
    }

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
    TxGeneratorWindow *gen = new TxGeneratorWindow(dock, backend());
    dock->setWidget(gen);
    parent->addDockWidget(Qt::BottomDockWidgetArea, dock);

    RawTxWindow *rawtx = parent->findChild<RawTxWindow*>();
    if (rawtx) {
        connect(gen, &TxGeneratorWindow::messageSelected, rawtx, &RawTxWindow::setMessage);
        connect(rawtx, &RawTxWindow::messageUpdated, gen, &TxGeneratorWindow::updateMessage);
    }

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
                       "https://github.com/Schildkroet/CANgaroo"
                       "\n"
                       "v " VERSION_STRING "\n"
                       "\n"
                       "(c)2015-2017 Hubert Denkmair\n"
                       "(c)2018-2022 Ethan Zonca\n"
                       "(c)2024 WeAct Studio\n"
                       "(c)2024-2026 Schildkroet\n"
                       "(c)2025 Wikilift\n"
                       "(c)2026 Jayachandran Dharuman"
                       "\n\n"
                       "CANgaroo is free software licensed"
                       "\nunder the GPL v2 license.");
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

    foreach (TxGeneratorWindow *gen, findChildren<TxGeneratorWindow*>()) {
        gen->stopAll();
    }
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
        QString filename = fileDialog.selectedFiles().at(0);
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
    m_languageMenu = new QMenu(tr("&Language"));
    QAction *aboutAction = ui->actionAbout;
    ui->menuHelp->insertMenu(aboutAction, m_languageMenu);

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
    /*TraceWindow *tw = currentTab()->findChild<TraceWindow *>();
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
    file.close();*/
}

void MainWindow::importFullTrace()
{
    /*TraceWindow *tw = currentTab()->findChild<TraceWindow*>();
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
    agg->layoutChanged();*/
}

void MainWindow::showThemeDialog()
{
    QDialog *themeDialog = new QDialog(this);
    themeDialog->setWindowTitle(tr("Theme Selection"));
    themeDialog->setModal(true);
    themeDialog->setMinimumWidth(350);

    QVBoxLayout *mainLayout = new QVBoxLayout(themeDialog);

    // Label
    QLabel *label = new QLabel(tr("Select application theme:"), themeDialog);
    mainLayout->addWidget(label);

    // ComboBox with available styles
    QComboBox *styleComboBox = new QComboBox(themeDialog);
    QStringList availableStyles = QStyleFactory::keys();
    styleComboBox->addItems(availableStyles);

    // Set current style as selected
    QString currentStyle = QApplication::style()->objectName();
    int currentIndex = -1;
    for (int i = 0; i < availableStyles.size(); ++i)
    {
        if (availableStyles[i].compare(currentStyle, Qt::CaseInsensitive) == 0)
        {
            currentIndex = i;
            break;
        }
    }
    if (currentIndex >= 0)
    {
        styleComboBox->setCurrentIndex(currentIndex);
    }

    mainLayout->addWidget(styleComboBox);

    // Info label
    QLabel *infoLabel = new QLabel(tr("Current style: %1").arg(currentStyle), themeDialog);
    infoLabel->setStyleSheet("color: gray; font-size: 10px;");
    mainLayout->addWidget(infoLabel);

    mainLayout->addSpacing(20);

    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        themeDialog);

    connect(buttonBox, &QDialogButtonBox::accepted, themeDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, themeDialog, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    // Connect style change preview
    connect(styleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [styleComboBox, infoLabel](int index)
            {
                QString selectedStyle = styleComboBox->itemText(index);
                infoLabel->setText(QObject::tr("Selected: %1").arg(selectedStyle));
            });

    // Execute dialog
    if (themeDialog->exec() == QDialog::Accepted)
    {
        QString selectedStyle = styleComboBox->currentText();

        // Apply the selected style
        QApplication::setStyle(QStyleFactory::create(selectedStyle));

        // Save to settings
        settings.setValue("ui/applicationStyle", selectedStyle);

        QMessageBox::information(this,
                                 tr("Theme Changed"),
                                 tr("The theme has been changed to %1.\n"
                                    "Some changes may require an application restart.").arg(selectedStyle));
    }

    themeDialog->deleteLater();
}

