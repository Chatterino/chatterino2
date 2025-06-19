#pragma once

#include "widgets/settingspages/GeneralPageView.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class ExternalToolsPage : public SettingsPage
{
public:
    ExternalToolsPage();

    bool filterElements(const QString &query) override;

private:
    void initLayout(GeneralPageView &layout);
    void exportSettings();
    void importSettings();

    GeneralPageView *view;
};

}  // namespace chatterino
