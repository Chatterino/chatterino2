#pragma once

#include "util/SignalListener.hpp"

#include <QLabel>
#include <QWidget>

namespace chatterino {

class FontSettingWidget : public QWidget
{
public:
    FontSettingWidget(QWidget *parent = nullptr);

private:
    void updateCurrentLabel();
    void showDialog();

    QLabel *currentLabel;
    SignalListener listener;
};

}  // namespace chatterino
