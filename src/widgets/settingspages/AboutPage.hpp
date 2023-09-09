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
    void addLicense(QFormLayout *form, const QString &name_,
                    const QString &website, const QString &licenseLink);

    QLabel *logo_{};
};

}  // namespace chatterino
