#include "ConfigurableWidget.h"
#include <QDomDocument>
#include <core/Backend.h>
#include <QEvent>
#include <QDockWidget>
#include <QTabWidget>
#include <QMdiSubWindow>

ConfigurableWidget::ConfigurableWidget(QWidget *parent) : QWidget(parent)
{
}

void ConfigurableWidget::changeEvent(QEvent *event)
{
    // installTranslator called -> LanguageChange
    if (event->type() == QEvent::LanguageChange) {

        retranslateUi();
        syncTitles();
    }
    QWidget::changeEvent(event);
}

void ConfigurableWidget::syncTitles()
{
    QString newTitle = windowTitle();
    if (newTitle.isEmpty()) return;

    // "child" is for determining which tab does current
    // widget belong to when finding its parent TabWidget
    QWidget *child = this;
    QWidget *p = parentWidget();

    while (p) {
        // MDI window (e.g. Trace window)
        if (QMdiSubWindow *sub = qobject_cast<QMdiSubWindow*>(p)) {
            sub->setWindowTitle(newTitle);
        }

        // Docking window (e.g. CanStatus, Log window)
        if (QDockWidget *dock = qobject_cast<QDockWidget*>(p)) {
            dock->setWindowTitle(newTitle);
        }

        // Tab window (e.g. Tab in MainWindow)
        if (QTabWidget *tabs = qobject_cast<QTabWidget*>(p)) {
            int index = tabs->indexOf(child);
            if (index != -1) {
                tabs->setTabText(index, newTitle);
            }
        }

        // continue to find the parent widget until reached the top
        child = p;
        p = p->parentWidget();
    }
}

bool ConfigurableWidget::saveXML(Backend &backend, QDomDocument &xml, QDomElement &root)
{
    (void) backend; (void) xml; (void) root;
    return true;
}

bool ConfigurableWidget::loadXML(Backend &backend, QDomElement &el)
{
    (void) backend; (void) el;
    return true;
}
