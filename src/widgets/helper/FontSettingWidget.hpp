#pragma once

#include <QLabel>

namespace chatterino {

class FontSettingWidget : public QWidget
{
public:
    FontSettingWidget(QWidget *parent = nullptr);

private:
    void updateCurrentLabel();
    QLabel *currentLabel;
};

}  // namespace chatterino
