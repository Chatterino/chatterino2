#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "util/LayoutCreator.hpp"
#    include "widgets/settingspages/SettingsPage.hpp"

#    include <QDebug>
#    include <QFormLayout>
#    include <QGroupBox>
#    include <QWidget>

namespace chatterino {
class Plugin;

class PluginsPage : public SettingsPage
{
public:
    PluginsPage();

private:
    void rebuildContent();

    LayoutCreator<QWidget> scrollAreaWidget_;
    QGroupBox *generalGroup;
    QFrame *dataFrame_;
};

}  // namespace chatterino

#endif
