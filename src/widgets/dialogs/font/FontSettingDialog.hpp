#pragma once

#include "common/ChatterinoSetting.hpp"
#include "widgets/dialogs/font/FontDialog.hpp"

#include <QObject>

namespace chatterino {

class FontSettingDialog : public FontDialog
{
    Q_OBJECT

public:
    FontSettingDialog(QStringSetting &family, IntSetting &size,
                      IntSetting &weight, QWidget *parent = nullptr);

private:
    /// Apply the current dialog's values to the font settings
    void setSettings();

    /// Restore the original setting values to the font settings
    void restoreSettings();

    QStringSetting &familySetting;
    IntSetting &sizeSetting;
    IntSetting &weightSetting;

    QString oldFamily;
    int oldSize;
    int oldWeight;
};

}  // namespace chatterino
