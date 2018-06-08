#pragma once

#include "widgets/settingspages/settingspage.hpp"

class QLabel;
class QFormLayout;

namespace chatterino {
namespace widgets {
namespace settingspages {

class AboutPage : public SettingsPage
{
public:
    AboutPage();

private:
    QLabel *logo;

    void addLicense(QFormLayout *form, const QString &name, const QString &website,
                    const QString &licenseLink);
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
