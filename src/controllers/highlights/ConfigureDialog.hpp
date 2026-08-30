// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/types/All.hpp"
#include "widgets/BasePopup.hpp"

#include <QWidget>

namespace chatterino::highlights {

class ConfigureDialog : public BasePopup
{
    Q_OBJECT

public:
    ConfigureDialog(AllHighlights _data, QWidget *parent);

    Q_SIGNAL void confirmed(AllHighlights data);

private:
    AllHighlights data;
    int previousSoundIndex;
};

}  // namespace chatterino::highlights
