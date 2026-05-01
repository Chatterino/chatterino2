// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QFont>
#include <QListWidget>
#include <QString>
#include <QWidget>

namespace chatterino {

/// FontWeightWidget shows a list of available font weights for a given font family.
class FontWeightWidget : public QWidget
{
    Q_OBJECT

public:
    FontWeightWidget(const QFont &startFont, QWidget *parent = nullptr);

    /// Update the font family, resetting the selected font weight to the weight
    /// that's closest to "normal".
    void setFamily(const QString &family);

    /// Gets the currently selected font weight.
    int getSelected() const;

Q_SIGNALS:
    void selectedChanged();

private:
    QListWidget *list;
};

}  // namespace chatterino
