// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BaseWindow.hpp"

#include <QString>
#include <QWidget>

namespace chatterino {

class TwitchChannel;

class ChatterListWidget : public BaseWindow
{
    Q_OBJECT

public:
    ChatterListWidget(const TwitchChannel *twitchChannel, QWidget *parent);

    Q_SIGNAL void userClicked(QString userLogin);

private:
    const TwitchChannel *twitchChannel_ = nullptr;
    QLabel *loadingLabel_ = nullptr;
    QVBoxLayout *dockVbox_ = nullptr;
    QLineEdit *searchBar_ = nullptr;
    QListWidget *resultList_ = nullptr;
    QListWidget *chattersList_ = nullptr;

    QStringList chatterList_;
    QStringList modChatters_;
    QStringList vipChatters_;

    void refresh();
    void setupUi();
    void clearUi();
};

}  // namespace chatterino
