// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "widgets/dialogs/RemotePluginDialog.hpp"

#    include "Application.hpp"
#    include "common/network/NetworkResult.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/RemotePlugin.hpp"
#    include "singletons/Settings.hpp"
#    include "widgets/helper/Line.hpp"

#    include <QInputDialog>
#    include <QLabel>
#    include <QMessageBox>
#    include <QPushButton>
#    include <QStringListModel>
#    include <QVBoxLayout>

#    include <algorithm>
#    include <span>

namespace {

using namespace Qt::Literals;
using namespace chatterino;

QString joinStrings(std::span<const QString> authors)
{
    QString str;
    bool first = true;
    for (const auto &author : authors)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            str += u", "_s;
        }
        str += author;
    }
    return str;
}

QString joinPermissions(std::span<const PluginPermission> permissions)
{
    QString str;
    bool first = true;
    for (const auto &permission : permissions)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            str += u", "_s;
        }

        switch (permission.type)
        {
            case PluginPermission::Type::FilesystemRead:
                str += u"filesystem read";
                break;
            case PluginPermission::Type::FilesystemWrite:
                str += u"filesystem write";
                break;
            case PluginPermission::Type::Network:
                str += u"network";
                break;
        }
    }
    return str;
}

}  // namespace

namespace chatterino {

RemotePluginDialog::RemotePluginDialog(RemotePluginPtr remotePlugin,
                                       QWidget *parent)
    : QDialog(parent)
    , vbox(this)
    , boldFont(this->font())
    , remotePlugin(std::move(remotePlugin))
{
    this->boldFont.setBold(true);
    this->setMinimumWidth(250);
    this->setAttribute(Qt::WA_DeleteOnClose);

    QFont titleFont = this->font();
    titleFont.setBold(true);
    titleFont.setPointSize(14);

    const auto &meta = this->remotePlugin->meta;
    this->setWindowTitle(meta.name);

    auto *nameLbl = new QLabel(meta.name);
    nameLbl->setFont(titleFont);
    this->vbox.addWidget(nameLbl);
    if (!meta.description.isEmpty())
    {
        auto *descriptionLbl = new QLabel(meta.description);
        descriptionLbl->setTextFormat(Qt::MarkdownText);
        descriptionLbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
        descriptionLbl->setOpenExternalLinks(true);
        descriptionLbl->setWordWrap(true);
        this->vbox.addWidget(descriptionLbl);
        this->vbox.addSpacing(2);
    }

    this->vbox.addWidget(new Line(false));

    this->appendField(u"ID"_s, this->remotePlugin->id);
    this->appendField(u"Version"_s,
                      QString::fromStdString(meta.version.to_string()));
    this->appendField(u"Authors"_s, joinStrings(meta.authors));
    if (!meta.homepage.isEmpty())
    {
        auto escaped = meta.homepage.toHtmlEscaped();
        this->appendField(u"Homepage"_s,
                          u"<a href=\"" % escaped % u"\">" % escaped % u"</a>",
                          true);
    }
    this->appendField(u"License"_s, meta.license);
    this->appendField(u"Tags"_s, joinStrings(meta.tags));
    this->appendField(u"Permissions"_s, joinPermissions(meta.permissions));

    this->vbox.addWidget(new Line(false));
    this->vbox.addStretch();

    auto *wrap = new QWidget;
    wrap->setContentsMargins({});
    wrap->setLayout(&this->buttonLayout);
    this->vbox.addWidget(wrap);

    getSettings()->enabledPlugins.connect(
        [this] {
            if (this->enabledRefreshQueued)
            {
                return;
            }
            this->enabledRefreshQueued = true;
            QMetaObject::invokeMethod(
                this,
                [this] {
                    this->enabledRefreshQueued = false;
                    this->refreshButtons();
                },
                Qt::QueuedConnection);
        },
        this->connections_, false);
    this->connections_.emplace_back(
        getApp()->getPlugins()->onPluginsUpdated.connect([this] {
            this->refreshButtons();
        }));

    this->refreshButtons();
}

RemotePluginDialog::~RemotePluginDialog() = default;

void RemotePluginDialog::appendField(const QString &name, const QString &value,
                                     bool html)
{
    if (value.isEmpty())
    {
        return;
    }

    auto *nameLbl = new QLabel(name);
    nameLbl->setFont(this->boldFont);
    this->vbox.addWidget(nameLbl);

    auto *valueLbl = new QLabel(value);
    if (html)
    {
        valueLbl->setTextFormat(Qt::RichText);
        valueLbl->setOpenExternalLinks(true);
        valueLbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
    }
    else
    {
        valueLbl->setTextFormat(Qt::PlainText);
    }
    valueLbl->setWordWrap(true);
    this->vbox.addWidget(valueLbl);
    this->vbox.addSpacing(1);
}

void RemotePluginDialog::refreshButtons()
{
    // Clear layout
    {
        const QLayoutItem *child = nullptr;
        while ((child = this->buttonLayout.takeAt(0)) != nullptr)
        {
            delete child->widget();
            delete child;
        }
    }

    auto addButton = [&](auto &&name, auto &&fn) {
        auto *button = new QPushButton(std::forward<decltype(name)>(name));
        QObject::connect(button, &QPushButton::clicked, this,
                         std::forward<decltype(fn)>(fn));
        this->buttonLayout.addWidget(button);
        return button;
    };

    auto *existing =
        getApp()->getPlugins()->getPluginByID(this->remotePlugin->id);
    bool related = existing != nullptr &&
                   existing->meta.isRelatedTo(this->remotePlugin->meta);

    QString installName;
    bool installIsUpdate = false;
    if (existing && related)
    {
        if (existing->meta.version == this->remotePlugin->meta.version)
        {
            // Toggle
            auto toggleText = u"Disable"_s;
            if (!PluginController::isPluginEnabled(existing->id))
            {
                toggleText = u"Enable"_s;
            }
            addButton(toggleText, &RemotePluginDialog::togglePlugin);

            installName = u"Reinstall"_s;
        }
        else if (existing->meta.version < this->remotePlugin->meta.version)
        {
            installName =
                u"Update from "_s %
                QString::fromStdString(existing->meta.version.to_string());
        }
        else
        {
            installName =
                u"Downgrade from "_s %
                QString::fromStdString(existing->meta.version.to_string());
        }

        addButton("Uninstall", &RemotePluginDialog::uninstallPlugin);

        installIsUpdate = true;
    }
    else
    {
        installName = u"Install"_s;
    }

    this->installButton = addButton(installName, [this, installIsUpdate] {
        this->installPlugin(installIsUpdate);
    });
}

void RemotePluginDialog::togglePlugin()
{
    auto *existing =
        getApp()->getPlugins()->getPluginByID(this->remotePlugin->id);
    if (!existing)
    {
        assert(false);
        return;
    }

    auto vec = getSettings()->enabledPlugins.getValue();
    auto it = std::ranges::find(vec, this->remotePlugin->id);
    if (it == vec.end())
    {
        vec.emplace_back(this->remotePlugin->id);
    }
    else
    {
        vec.erase(it);
    }
    getSettings()->enabledPlugins = std::move(vec);
    getApp()->getPlugins()->reload(this->remotePlugin->id);
}

void RemotePluginDialog::installPlugin(bool update)
{
    if (this->installButton)
    {
        this->installButton->setDisabled(true);
    }

    getApp()->getPlugins()->download({
        .remotePlugin = this->remotePlugin,
        .onExistingOverwrite =
            [&] {
                auto res = QMessageBox::warning(
                    this, "Chatterino",
                    u"A plugin with the ID '" % this->remotePlugin->id %
                        "' already exists.\nDo you want to "
                        "replace it?",
                    QMessageBox::Yes | QMessageBox::No);
                return res == QMessageBox::Yes;
            },
        .onDone =
            [self = QPointer(this)](ExpectedStr<void> res) {
                if (!self)
                {
                    return;
                }
                if (self->installButton)
                {
                    self->installButton->setDisabled(false);
                }

                if (!res)
                {
                    QMessageBox::warning(
                        self, "Chatterino",
                        u"Failed to install plugin: " % res.error());
                }
            },
        .update = update,
    });
}

void RemotePluginDialog::uninstallPlugin()
{
    auto *mb = new QMessageBox(
        QMessageBox::Question, "Chatterino",
        u"Are you sure you want to remove '" %
            this->remotePlugin->meta.name.toHtmlEscaped() %
            "'?<br>"
            "<b>Keep Data</b> will remove the plugin code but keep its `/data` "
            "directory.<br>"
            "<b>Remove Everything</b> will remove all code and data.",
        QMessageBox::Cancel, this);
    mb->setTextFormat(Qt::RichText);
    auto *keepData = mb->addButton("Keep Data", QMessageBox::DestructiveRole);
    auto *removeEverything =
        mb->addButton("Remove Everything", QMessageBox::DestructiveRole);

    QObject::connect(keepData, &QPushButton::clicked, this, [this] {
        this->doUninstallPlugin(false);
    });
    QObject::connect(removeEverything, &QPushButton::clicked, this, [this] {
        this->doUninstallPlugin(true);
    });
    mb->show();
}

void RemotePluginDialog::doUninstallPlugin(bool eraseData)
{
    auto res =
        getApp()->getPlugins()->removePlugin(this->remotePlugin->id, eraseData);
    if (!res)
    {
        QMessageBox::warning(this, "Chatterino",
                             "Failed to remove plugin: " % res.error());
    }
}

}  // namespace chatterino

#endif
