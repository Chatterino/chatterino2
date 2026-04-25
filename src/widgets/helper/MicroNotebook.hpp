// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QHBoxLayout>
#include <QStackedLayout>
#include <QString>
#include <QWidget>

#include <vector>

namespace chatterino {

/// An extremely simple implementation of the regular notebook.
///
/// It's essentially a QTabWidget without the downside that the fusion style
/// provides poor contrast with it.
class MicroNotebook : public QWidget
{
public:
    MicroNotebook(QWidget *parent = nullptr);

    int addPage(QWidget *page, QString name);

    void select(QWidget *page);

    bool isSelected(QWidget *page) const;

    void setShowHeader(bool showHeader);

private:
    struct Item {
        QString name;
        int index;
    };
    std::vector<Item> items;
    QStackedLayout layout;
    QHBoxLayout topBar;
    QWidget *topWidget = nullptr;
    QWidget *horizontalSeparator = nullptr;
};

}  // namespace chatterino
