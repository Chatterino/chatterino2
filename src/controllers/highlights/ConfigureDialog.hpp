// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/types/AnyHighlight.hpp"
#include "widgets/BasePopup.hpp"

#include <QWidget>

namespace chatterino::highlights {

class ConfigureDialog : public BasePopup
{
    Q_OBJECT

public:
    ConfigureDialog(AnyHighlight _data, QWidget *parent);

    Q_SIGNAL void confirmed(chatterino::highlights::AnyHighlight data);

private:
    AnyHighlight data;
    int previousSoundIndex;
};

}  // namespace chatterino::highlights
