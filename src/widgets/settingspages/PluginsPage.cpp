#ifdef CHATTERINO_HAVE_PLUGINS
#    include "widgets/settingspages/PluginsPage.hpp"

#    include "Application.hpp"
#    include "common/Args.hpp"
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
        auto *scaryLabel = new QLabel(
            "Plugins can expand functionality of "
            "Chatterino. They can be made in Lua. This functionality is "
            "still in public alpha stage. Use ONLY the plugins you trust. "
            "The permission system is best effort, always "
            "assume plugins can bypass permissions and can execute "
            "arbitrary code. To see how to create plugins " +
            formatRichNamedLink("https://github.com/Chatterino/chatterino2/"
                                "blob/master/docs/wip-plugins.md",
                                "look at the manual") +
            ".");
        scaryLabel->setWordWrap(true);
        scaryLabel->setOpenExternalLinks(true);
        groupLayout->addRow(scaryLabel);

        auto *description =
            new QLabel("You can load plugins by putting them into " +
                       formatRichNamedLink(
                           "file:///" + getApp()->getPaths().pluginsDirectory,
                           "the Plugins directory") +
                       ". Each one is a new directory.");
        description->setOpenExternalLinks(true);
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");
        groupLayout->addRow(description);

        auto *box = this->createCheckBox("Enable plugins",
                                         getSettings()->pluginsEnabled);
        QObject::connect(box, &QCheckBox::released, [this]() {
            this->rebuildContent();
        });
        groupLayout->addRow(box);
        if (getApp()->getArgs().safeMode)
        {
            box->setEnabled(false);
            auto *disabledLabel = new QLabel(this);
            disabledLabel->setText("Plugins will not be fully loaded because "
                                   "Chatterino is in safe mode. You can still "
                                   "enable and disable them.");
            groupLayout->addRow(disabledLabel);
        }
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
    layout->setParent(this->dataFrame_);
    for (const auto &[id, plugin] : getApp()->getPlugins()->plugins())
    {
        auto groupHeaderText =
            QString("%1 (%2, from %3)")
                .arg(plugin->meta.name,
                     QString::fromStdString(plugin->meta.version.to_string()),
                     id);
        auto groupBox = layout.emplace<QGroupBox>(groupHeaderText);
        groupBox->setParent(this->dataFrame_);
        auto pluginEntry = groupBox.setLayoutType<QFormLayout>();
        pluginEntry->setParent(groupBox.getElement());

        if (!plugin->meta.isValid())
        {
            QString errors = "<ul>";
            for (const auto &err : plugin->meta.errors)
            {
                errors += "<li>" + err.toHtmlEscaped() + "</li>";
            }
            errors += "</ul>";

            auto *warningLabel = new QLabel(
                "There were errors while loading metadata for this plugin:" +
                    errors,
                this->dataFrame_);
            warningLabel->setTextFormat(Qt::RichText);
            warningLabel->setStyleSheet("color: #f00");
            pluginEntry->addRow(warningLabel);
        }
        if (!plugin->error().isNull())
        {
            auto *errorLabel =
                new QLabel("There was an error while loading this plugin: " +
                               plugin->error(),
                           this->dataFrame_);
            errorLabel->setStyleSheet("color: #f00");
            errorLabel->setWordWrap(true);
            pluginEntry->addRow(errorLabel);
        }

        auto *description =
            new QLabel(plugin->meta.description, this->dataFrame_);
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");
        pluginEntry->addRow(description);

        QString authorsTxt;
        for (const auto &author : plugin->meta.authors)
        {
            if (!authorsTxt.isEmpty())
            {
                authorsTxt += ", ";
            }

            authorsTxt += author;
        }
        pluginEntry->addRow("Authors",
                            new QLabel(authorsTxt, this->dataFrame_));

        if (!plugin->meta.homepage.isEmpty())
        {
            auto *homepage = new QLabel(formatRichLink(plugin->meta.homepage),
                                        this->dataFrame_);
            homepage->setOpenExternalLinks(true);
            pluginEntry->addRow("Homepage", homepage);
        }
        pluginEntry->addRow("License",
                            new QLabel(plugin->meta.license, this->dataFrame_));

        QString commandsTxt;
        for (const auto &cmdName : plugin->listRegisteredCommands())
        {
            if (!commandsTxt.isEmpty())
            {
                commandsTxt += ", ";
            }

            commandsTxt += cmdName;
        }
        pluginEntry->addRow("Commands",
                            new QLabel(commandsTxt, this->dataFrame_));
        if (!plugin->meta.permissions.empty())
        {
            QString perms = "<ul>";
            for (const auto &perm : plugin->meta.permissions)
            {
                perms += "<li>" + perm.toHtml() + "</li>";
            }
            perms += "</ul>";

            auto *lbl =
                new QLabel("Required permissions:" + perms, this->dataFrame_);
            lbl->setTextFormat(Qt::RichText);
            pluginEntry->addRow(lbl);
        }

        if (plugin->meta.isValid())
        {
            QString toggleTxt = "Enable";
            if (PluginController::isPluginEnabled(id))
            {
                toggleTxt = "Disable";
            }

            auto *toggleButton = new QPushButton(toggleTxt, this->dataFrame_);
            QObject::connect(
                toggleButton, &QPushButton::pressed, [name = id, this]() {
                    std::vector<QString> val =
                        getSettings()->enabledPlugins.getValue();
                    if (PluginController::isPluginEnabled(name))
                    {
                        val.erase(std::remove(val.begin(), val.end(), name),
                                  val.end());
                    }
                    else
                    {
                        val.push_back(name);
                    }
                    getSettings()->enabledPlugins.setValue(val);
                    getApp()->getPlugins()->reload(name);
                    this->rebuildContent();
                });
            pluginEntry->addRow(toggleButton);
        }

        auto *reloadButton = new QPushButton("Reload", this->dataFrame_);
        QObject::connect(reloadButton, &QPushButton::pressed,
                         [name = id, this]() {
                             getApp()->getPlugins()->reload(name);
                             this->rebuildContent();
                         });
        pluginEntry->addRow(reloadButton);
        if (getApp()->getArgs().safeMode)
        {
            reloadButton->setEnabled(false);
        }
    }
}

}  // namespace chatterino

#endif
