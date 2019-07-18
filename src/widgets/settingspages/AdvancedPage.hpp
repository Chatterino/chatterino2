#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class AdvancedPage : public SettingsPage
{
public:
    AdvancedPage();

    void timeoutDurationChanged(QLineEdit &durationPerUnit, const QString &unit,
                                QSpinBox &durationInSec,
                                QStringSetting settingDurationInUnit);

    void calculatedDurationChanged(const int durationInSec,
                                   IntSetting settingDurationInSec);

    void timeoutUnitChanged(const QString newUnit, QStringSetting settingUnit);

private:
};

}  // namespace chatterino
