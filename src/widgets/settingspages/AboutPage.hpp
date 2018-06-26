#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

class QLabel;
class QFormLayout;

namespace chatterino {

class AboutPage : public SettingsPage
{
public:
    AboutPage();

private:
    QLabel *logo;

    void addLicense(QFormLayout *form, const QString &name, const QString &website,
                    const QString &licenseLink);
};

}  // namespace chatterino
