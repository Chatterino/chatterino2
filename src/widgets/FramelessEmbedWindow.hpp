// SPDX-FileCopyrightText: 2021 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BaseWindow.hpp"

namespace chatterino {

class Split;

class FramelessEmbedWindow : public BaseWindow
{
public:
    FramelessEmbedWindow();

protected:
#ifdef USEWINSDK

    bool nativeEvent(const QByteArray &eventType, void *message,
                     qintptr *result) override;

    void showEvent(QShowEvent *event) override;
#endif

private:
    Split *split_{};
};

}  // namespace chatterino
