#include "pajlada/settings/settinglistener.hpp"

#include <QLabel>

namespace chatterino {

class FontSettingWidget : public QWidget
{
    QLabel *currentLabel;
    pajlada::SettingListener listener;

    void updateCurrentLabel();

public:
    FontSettingWidget(QWidget *parent = nullptr);
};

}  // namespace chatterino
