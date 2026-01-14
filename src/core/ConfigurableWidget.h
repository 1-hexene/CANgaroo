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

/*
  修改后的基类头文件，增加了事件处理和同步函数
*/

#pragma once

#include <QObject>
#include <QWidget>
#include <QEvent>

class Backend;
class QDomDocument;
class QDomElement;

class ConfigurableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigurableWidget(QWidget *parent = 0);
    virtual bool saveXML(Backend &backend, QDomDocument &xml, QDomElement &root);
    virtual bool loadXML(Backend &backend, QDomElement &el);

protected:
    // 捕获语言切换事件
    void changeEvent(QEvent *event) override;

    // 供子类实现的 UI 翻译钩子
    virtual void retranslateUi() {}

    // 自动同步标题到外层容器（Dock 或 Tab）
    void syncTitles();

signals:
    void settingsChanged(ConfigurableWidget *sender);
};
