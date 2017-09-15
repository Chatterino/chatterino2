#pragma once

#include <QWidget>

#include "channel.hpp"

namespace chatterino {
namespace widgets {

class EmotePopup : public QWidget
{
public:
    explicit EmotePopup(QWidget *parent = 0);

    void loadChannel(std::shared_ptr<Channel> channel);
};
}
}
