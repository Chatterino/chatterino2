#include "PluginsPage.hpp"
#ifdef CHATTERINO_HAVE_PLUGINS

#    include "Application.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "singletons/Paths.hpp"
#    include "singletons/Settings.hpp"
#    include "util/Helpers.hpp"
#    include "util/LayoutCreator.hpp"
#    include "util/RemoveScrollAreaBackground.hpp"

#    include <QCheckBox>
#    include <QFormLayout>
#    include <QGroupBox>
#    include <QLabel>
#    include <qobject.h>
#    include <QObject>
#    include <QPushButton>
#    include <QWidget>

namespace chatterino {

PluginsPage::PluginsPage()
    : scrollAreaWidget_(nullptr)
    , dataFrame_(nullptr)
{
    LayoutCreator<PluginsPage> layoutCreator(this);
    auto scrollArea = layoutCreator.emplace<QScrollArea>();

    auto widget = scrollArea.emplaceScrollAreaWidget();
    this->scrollAreaWidget_ = widget;
    removeScrollAreaBackground(scrollArea.getElement(), widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();

    {
        auto group = layout.emplace<QGroupBox>("General plugin settings");
        this->generalGroup = group.getElement();
        auto groupLayout = group.setLayoutType<QFormLayout>();
        auto *description = new QLabel(
            "You can load plugins by putting them into " +
            formatRichNamedLink("file:///" + getPaths()->pluginsDirectory,
                                "the Plugins directory") +
            ". Each one is a new directory.");
        description->setOpenExternalLinks(true);
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");
        groupLayout->addRow(description);

        auto *box = this->createCheckBox("Enable plugins",
                                         getSettings()->enableAnyPlugins);
        QObject::connect(box, &QCheckBox::released, [this]() {
            this->rebuildContent();
        });
        groupLayout->addRow(box);
    }

    this->rebuildContent();
}

void PluginsPage::rebuildContent()
{
    if (this->dataFrame_ != nullptr)
    {
        this->dataFrame_->deleteLater();
        this->dataFrame_ = nullptr;
    }
    auto frame = LayoutCreator<QFrame>(new QFrame(this));
    this->dataFrame_ = frame.getElement();
    this->scrollAreaWidget_.append(this->dataFrame_);
    auto layout = frame.setLayoutType<QVBoxLayout>();
    for (const auto &[codename, plugin] : getApp()->plugins->plugins())
    {
        auto headerText = QString("%1 (%2)").arg(
            plugin->meta.name,
            QString::fromStdString(plugin->meta.version.to_string()));
        if (plugin->isDupeName)
        {
            // add ", from <folder name>)" in place of ")"
            headerText.chop(1);
            headerText += ", from " + codename + ")";
        }
        auto plgroup = layout.emplace<QGroupBox>(headerText);
        auto pl = plgroup.setLayoutType<QFormLayout>();
        auto *descrText = new QLabel(plugin->meta.description);
        descrText->setWordWrap(true);
        descrText->setStyleSheet("color: #bbb");
        pl->addRow(descrText);
        pl->addRow("Authors", new QLabel(plugin->meta.authors));
        auto *homepage = new QLabel(formatRichLink(plugin->meta.homepage));
        homepage->setOpenExternalLinks(true);

        pl->addRow("Homepage", homepage);
        pl->addRow("License", new QLabel(plugin->meta.license));

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

        QString enableOrDisableStr = "Enable";
        if (getApp()->plugins->isEnabled(codename))
        {
            enableOrDisableStr = "Disable";
        }

        auto *enableDisable = new QPushButton(enableOrDisableStr);
        QObject::connect(
            enableDisable, &QPushButton::pressed, [name = codename, this]() {
                std::vector<QString> val =
                    getSettings()->enabledPlugins.getValue();
                if (getApp()->plugins->isEnabled(name))
                {
                    val.erase(std::remove(val.begin(), val.end(), name),
                              val.end());
                }
                else
                {
                    val.push_back(name);
                }
                getSettings()->enabledPlugins.setValue(val);
                getApp()->plugins->reload(name);
                this->rebuildContent();
            });
        pl->addRow(enableDisable);

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
#endif
