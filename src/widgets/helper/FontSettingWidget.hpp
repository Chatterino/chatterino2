#include <QLabel>

namespace chatterino {

class FontSettingWidget : public QWidget
{
    QLabel *currentLabel;

    void updateCurrentLabel();

public:
    FontSettingWidget(QWidget *parent = nullptr);
};

}  // namespace chatterino
