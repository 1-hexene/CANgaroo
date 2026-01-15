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

    // child 用于在向上寻找 TabWidget 时确定当前组件属于哪个页签
    QWidget *child = this;
    QWidget *p = parentWidget();

    while (p) {
        // 1. 处理 MDI 子窗口 (Trace Window 常属于此类)
        if (QMdiSubWindow *sub = qobject_cast<QMdiSubWindow*>(p)) {
            sub->setWindowTitle(newTitle);
        }

        // 2. 处理停靠窗口 (CanStatus, Log 视图)
        if (QDockWidget *dock = qobject_cast<QDockWidget*>(p)) {
            dock->setWindowTitle(newTitle);
        }

        // 3. 处理标签页容器 (MainWindow 的中心 Tab 区域)
        if (QTabWidget *tabs = qobject_cast<QTabWidget*>(p)) {
            int index = tabs->indexOf(child);
            if (index != -1) {
                tabs->setTabText(index, newTitle);
            }
        }

        // 继续向上寻找，直到顶层
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
