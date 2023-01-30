#include "PluginsPage.hpp"

#include "Application.hpp"
#include "controllers/plugins/PluginController.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

namespace chatterino {

PluginsPage::PluginsPage()
{
    LayoutCreator<PluginsPage> layoutCreator(this);

    auto scroll = layoutCreator.emplace<QScrollArea>();
    auto widget = scroll.emplaceScrollAreaWidget();
    removeScrollAreaBackground(scroll.getElement(), widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();
    auto group = layout.emplace<QGroupBox>("Plugins");
    auto groupLayout = group.setLayoutType<QFormLayout>();

    auto *description =
        new QLabel("You can load plugins by putting them into "
                   "<chatterino-app-data-folder>/Plugins/. Each one is a "
                   "new directory.");
    description->setWordWrap(true);
    description->setStyleSheet("color: #bbb");
    groupLayout->addRow(description);

    for (const auto &[codename, plugin] : getApp()->plugins->plugins())
    {
        auto plgroup = groupLayout.emplace<QGroupBox>(plugin->meta.name);
        auto pl = plgroup.setLayoutType<QFormLayout>();
        auto *descrText = new QLabel(plugin->meta.description);
        descrText->setTextFormat(Qt::TextFormat::MarkdownText);
        descrText->setWordWrap(true);
        descrText->setStyleSheet("color: #bbb");
        pl->addRow(descrText);
        pl->addRow("Authors", new QLabel(plugin->meta.authors));
        auto *homepage = new QLabel(formatRichLink(plugin->meta.homepage));
        homepage->setOpenExternalLinks(true);

        pl->addRow("Homepage", homepage);

        auto *reload = new QPushButton("Reload");
        QObject::connect(reload, &QPushButton::pressed, [=]() {
            getApp()->plugins->reload(codename);
        });
        pl->addRow(reload);
    }
}
}  // namespace chatterino
