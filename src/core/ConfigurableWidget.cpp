#include "ConfigurableWidget.h"
#include <QDomDocument>
#include <core/Backend.h>
#include <QEvent>
#include <QDockWidget>
#include <QTabWidget>

ConfigurableWidget::ConfigurableWidget(QWidget *parent) : QWidget(parent)
{
}

void ConfigurableWidget::changeEvent(QEvent *event)
{
    // 当 installTranslator 被调用后，系统会发出 LanguageChange 事件
    if (event->type() == QEvent::LanguageChange) {

        // 1. 调用子类的刷新逻辑（子类需重写此函数，并在里面调用 setWindowTitle）
        retranslateUi();

        // 2. 将子类设置的新标题同步给外部的 QDockWidget 或 QTabWidget
        syncTitles();
    }
    QWidget::changeEvent(event);
}

void ConfigurableWidget::syncTitles()
{
    // 获取当前组件翻译后的 windowTitle
    QString newTitle = windowTitle();
    if (newTitle.isEmpty()) {
        return;
    }

    QWidget *p = parentWidget();
    while (p) {
        // 如果是在停靠窗口中（如 CAN Status, Log）
        if (QDockWidget *dock = qobject_cast<QDockWidget*>(p)) {
            dock->setWindowTitle(newTitle);
            break;
        }

        // 如果是在主标签页中（如 Trace Window）
        // 结构通常为：ConfigurableWidget -> QMainWindow -> QTabWidget
        if (p->parentWidget()) {
            if (QTabWidget *tabs = qobject_cast<QTabWidget*>(p->parentWidget())) {
                int index = tabs->indexOf(p);
                if (index != -1) {
                    tabs->setTabText(index, newTitle);
                    break;
                }
            }
        }
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
