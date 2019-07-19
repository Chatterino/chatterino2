#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class AdvancedPage : public SettingsPage
{
public:
    AdvancedPage();

private:
    // list needed for dynamic timeout settings
    QList<QLineEdit *> durationInputs;
    QList<QComboBox *> unitInputs;

    // iterators used in dynamic timeout settings
    QList<QLineEdit *>::iterator itDurationInput;
    QList<QComboBox *>::iterator itUnitInput;

private slots:
    void timeoutDurationChanged(const QString &newDuration);
    void timeoutUnitChanged(const QString &newUnit);
};

}  // namespace chatterino
