#pragma once
#include "util/LayoutCreator.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QFormLayout>
#include <QGroupBox>
namespace chatterino {
class Plugin;

class PluginsPage : public SettingsPage
{
public:
    PluginsPage();

private:
    void rebuildContent();

    LayoutCreator<QScrollArea> scrollArea_;
};
}  // namespace chatterino
