// SPDX-FileCopyrightText: 2020 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QDialog>

namespace chatterino {

class SelectChannelFiltersDialog : public QDialog
{
public:
    SelectChannelFiltersDialog(const QList<QUuid> &previousSelection,
                               bool previousAnyOf, QWidget *parent = nullptr);

    const QList<QUuid> &getSelection() const;
    bool getAnyOf() const;

private:
    QList<QUuid> currentSelection_;
    bool anyOf_;
};

}  // namespace chatterino
