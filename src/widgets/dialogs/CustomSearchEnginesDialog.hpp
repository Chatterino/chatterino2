// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QDialog>

namespace chatterino {

class CustomSearchEnginesDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit CustomSearchEnginesDialog(QWidget *parent = nullptr);
};

}  // namespace chatterino
