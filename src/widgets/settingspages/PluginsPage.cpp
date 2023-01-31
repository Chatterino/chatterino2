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
        //descrText->setTextFormat(Qt::TextFormat::MarkdownText);
        descrText->setWordWrap(true);
        descrText->setStyleSheet("color: #bbb");
        pl->addRow(descrText);
        pl->addRow("Authors", new QLabel(plugin->meta.authors));
        auto *homepage = new QLabel(formatRichLink(plugin->meta.homepage));
        homepage->setOpenExternalLinks(true);

        pl->addRow("Homepage", homepage);

        QString libString;
        bool hasDangerous = false;
        for (const auto &library : plugin->meta.libraryPermissions)
        {
            if (!libString.isEmpty())
            {
                libString += ", ";
            }
            if (library == "os" || library == "io" || library == "package")
            {
                hasDangerous = true;
            }
            libString += library;
        }
        if (hasDangerous)
        {
            libString += "\nDetected potentially dangerous libraries used, be "
                         "careful with this plugin";
        }
        auto *libs = new QLabel(libString);
        if (hasDangerous)
        {
            libs->setStyleSheet("color: red");
        }
        pl->addRow("Used libraries", libs);

        auto *reload = new QPushButton("Reload");
        QObject::connect(reload, &QPushButton::pressed, [name = codename]() {
            getApp()->plugins->reload(name);
        });
        pl->addRow(reload);
    }
}
}  // namespace chatterino
