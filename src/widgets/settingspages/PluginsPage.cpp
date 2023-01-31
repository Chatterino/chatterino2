#include "PluginsPage.hpp"

#include "Application.hpp"
#include "controllers/plugins/PluginController.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <qwidget.h>

namespace chatterino {

PluginsPage::PluginsPage()
    : scrollArea_(nullptr)
{
    LayoutCreator<PluginsPage> layoutCreator(this);
    this->scrollArea_ = layoutCreator.emplace<QScrollArea>();

    this->rebuildContent();
}

void PluginsPage::rebuildContent()
{
    auto widget = this->scrollArea_.emplaceScrollAreaWidget();
    removeScrollAreaBackground(this->scrollArea_.getElement(),
                               widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();
    auto group = layout.emplace<QGroupBox>("Plugins");
    auto groupLayout = group.setLayoutType<QFormLayout>();

    auto *description = new QLabel(
        "You can load plugins by putting them into " +
        formatRichNamedLink("file:///" + getPaths()->pluginsDirectory,
                            "the Plugins directory") +
        ". Each one is a "
        "new directory.");
    description->setOpenExternalLinks(true);
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

        QString cmds;
        for (const auto &cmdName : plugin->listRegisteredCommands())
        {
            if (!cmds.isEmpty())
            {
                cmds += ", ";
            }

            cmds += cmdName;
        }
        pl->addRow("Commands", new QLabel(cmds));

        auto *reload = new QPushButton("Reload");
        QObject::connect(reload, &QPushButton::pressed,
                         [name = codename, this]() {
                             getApp()->plugins->reload(name);
                             this->rebuildContent();
                         });
        pl->addRow(reload);
    }
}
}  // namespace chatterino
