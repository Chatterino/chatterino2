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
    QList<QSpinBox *> durationsCalculated;

    // iterators used in dynamic timeout settings

    QList<QLineEdit *>::iterator itDurationInput;
    QList<QComboBox *>::iterator itUnitInput;
    QList<QSpinBox *>::iterator itDurationsCalculated;

private slots:
    void timeoutDurationChanged(const QString &newDuration);

    void calculatedDurationChanged(const int &newDurationInSec);

    void timeoutUnitChanged(const QString &newUnit);
};

}  // namespace chatterino
