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
};

}  // namespace chatterino
