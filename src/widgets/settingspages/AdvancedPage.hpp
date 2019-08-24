#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class AdvancedPage : public SettingsPage
{
public:
    AdvancedPage();

private:
    // list needed for dynamic timeout settings
    QList<QLineEdit *> durationInputs_;
    QList<QComboBox *> unitInputs_;

    // iterators used in dynamic timeout settings
    QList<QLineEdit *>::iterator itDurationInput_;
    QList<QComboBox *>::iterator itUnitInput_;

private slots:
    void timeoutDurationChanged(const QString &newDuration);
    void timeoutUnitChanged(const QString &newUnit);
};

}  // namespace chatterino
