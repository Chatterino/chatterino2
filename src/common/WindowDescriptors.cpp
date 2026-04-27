// SPDX-FileCopyrightText: 2020 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/WindowDescriptors.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "widgets/Window.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace chatterino {

namespace {

QJsonArray loadWindowArray(const QString &settingsPath)
{
    QFile file(settingsPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    QByteArray data = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(data);
    QJsonArray windows_arr = document.object().value("windows").toArray();
    return windows_arr;
}

const QList<QUuid> loadFilters(QJsonValue val)
{
    QList<QUuid> filterIds;

    if (!val.isUndefined())
    {
        const auto array = val.toArray();
        filterIds.reserve(array.size());
        for (const auto id : array)
        {
            filterIds.append(QUuid::fromString(id.toString()));
        }
    }

    return filterIds;
}

}  // namespace

void SplitDescriptor::loadFromJSON(SplitDescriptor &descriptor,
                                   const QJsonObject &root,
                                   const QJsonObject &data)
{
    descriptor.type_ = data.value("type").toString();
    descriptor.server_ = data.value("server").toInt(-1);
    descriptor.moderationMode_ = root.value("moderationMode").toBool();
    if (data.contains("channel"))
    {
        descriptor.channelName_ = data.value("channel").toString();
    }
    else
    {
        descriptor.channelName_ = data.value("name").toString();
    }
    descriptor.filters_ = loadFilters(root.value("filters"));

    auto spellOverride = root["checkSpelling"];
    if (spellOverride.isBool())
    {
        descriptor.spellCheckOverride = spellOverride.toBool();
    }
}

IndirectChannel SplitDescriptor::decodeChannel() const
{
    assertInGuiThread();

    if (this->type_ == "twitch")
    {
        return getApp()->getTwitch()->getOrAddChannel(this->channelName_);
    }
    else if (this->type_ == "mentions")
    {
        return getApp()->getTwitch()->getMentionsChannel();
    }
    else if (this->type_ == "watching")
    {
        return getApp()->getTwitch()->getWatchingChannel();
    }
    else if (this->type_ == "whispers")
    {
        return getApp()->getTwitch()->getWhispersChannel();
    }
    else if (this->type_ == "live")
    {
        return getApp()->getTwitch()->getLiveChannel();
    }
    else if (this->type_ == "automod")
    {
        return getApp()->getTwitch()->getAutomodChannel();
    }
    else if (this->type_ == "misc")
    {
        return getApp()->getTwitch()->getChannelOrEmpty(this->channelName_);
    }

    return Channel::getEmpty();
}

SplitNodeDescriptor SplitNodeDescriptor::loadFromJSON(const QJsonObject &root)
{
    SplitNodeDescriptor descriptor;
    SplitDescriptor::loadFromJSON(descriptor, root, root["data"].toObject());
    descriptor.flexH_ = root["flexh"].toDouble(1.0);
    descriptor.flexV_ = root["flexv"].toDouble(1.0);
    return descriptor;
}

ContainerNodeDescriptor ContainerNodeDescriptor::loadFromJSON(
    const QJsonObject &root)
{
    ContainerNodeDescriptor descriptor;

    descriptor.flexH_ = root.value("flexh").toDouble(1.0);
    descriptor.flexV_ = root.value("flexv").toDouble(1.0);

    descriptor.vertical_ = root.value("type").toString() == "vertical";

    const auto items = root.value("items").toArray();
    for (const auto val : items)
    {
        const auto obj = val.toObject();
        auto type = obj.value("type");
        if (type.toString() == "split")
        {
            descriptor.items_.emplace_back(
                SplitNodeDescriptor::loadFromJSON(obj));
        }
        else
        {
            descriptor.items_.emplace_back(
                ContainerNodeDescriptor::loadFromJSON(obj));
        }
    }

    return descriptor;
}

TabDescriptor TabDescriptor::loadFromJSON(const QJsonObject &tabObj)
{
    TabDescriptor tab;
    // Load tab custom title
    QJsonValue titleVal = tabObj.value("title");
    if (titleVal.isString())
    {
        tab.customTitle_ = titleVal.toString();
    }

    // Load tab selected state
    tab.selected_ = tabObj.value("selected").toBool(false);

    // Load tab "highlightsEnabled" state
    tab.highlightsEnabled_ = tabObj.value("highlightsEnabled").toBool(true);

    QJsonObject splitRoot = tabObj.value("splits2").toObject();

    // Load tab splits
    if (!splitRoot.isEmpty())
    {
        // root type
        auto nodeType = splitRoot.value("type").toString();
        if (nodeType == "split")
        {
            tab.rootNode_ = SplitNodeDescriptor::loadFromJSON(splitRoot);
        }
        else if (nodeType == "horizontal" || nodeType == "vertical")
        {
            tab.rootNode_ = ContainerNodeDescriptor::loadFromJSON(splitRoot);
        }
    }

    return tab;
}

WindowLayout WindowLayout::loadFromFile(const QString &path)
{
    WindowLayout layout;

    bool hasSetAMainWindow = false;

    // "deserialize"
    for (const auto windowVal : loadWindowArray(path))
    {
        QJsonObject windowObj = windowVal.toObject();

        WindowDescriptor window;

        // Load window type
        QString typeVal = windowObj.value("type").toString();
        auto type = typeVal == "main" ? WindowType::Main : WindowType::Popup;

        if (type == WindowType::Main)
        {
            if (hasSetAMainWindow)
            {
                qCDebug(chatterinoCommon)
                    << "Window Layout file contains more than one Main window "
                       "- demoting to Popup type";
                type = WindowType::Popup;
            }
            hasSetAMainWindow = true;
        }

        window.type_ = type;

        // Load window state
        if (windowObj.value("state") == "minimized")
        {
            window.state_ = WindowDescriptor::State::Minimized;
        }
        else if (windowObj.value("state") == "maximized")
        {
            window.state_ = WindowDescriptor::State::Maximized;
        }

        // Load window geometry
        {
            int x = windowObj.value("x").toInt(-1);
            int y = windowObj.value("y").toInt(-1);
            int width = windowObj.value("width").toInt(-1);
            int height = windowObj.value("height").toInt(-1);

            window.geometry_ = QRect(x, y, width, height);
        }

        bool hasSetASelectedTab = false;

        // Load window tabs
        QJsonArray tabs = windowObj.value("tabs").toArray();
        for (QJsonValue tabVal : tabs)
        {
            TabDescriptor tab = TabDescriptor::loadFromJSON(tabVal.toObject());
            if (tab.selected_)
            {
                if (hasSetASelectedTab)
                {
                    qCDebug(chatterinoCommon)
                        << "Window contains more than one selected tab - "
                           "demoting to unselected";
                    tab.selected_ = false;
                }
                hasSetASelectedTab = true;
            }
            window.tabs_.emplace_back(std::move(tab));
        }

        // Load emote popup position
        {
            auto emotePopup = windowObj["emotePopup"].toObject();
            layout.emotePopupBounds_ = QRect{
                emotePopup["x"].toInt(),
                emotePopup["y"].toInt(),
                emotePopup["width"].toInt(),
                emotePopup["height"].toInt(),
            };
        }

        layout.windows_.emplace_back(std::move(window));
    }

    return layout;
}

void WindowLayout::activateOrAddChannel(ProviderId provider,
                                        const QString &name)
{
    if (provider != ProviderId::Twitch || name.startsWith(u'/') ||
        name.startsWith(u'$'))
    {
        qCWarning(chatterinoWindowmanager)
            << "Only twitch channels can be set as active";
        return;
    }

    auto mainWindow = std::find_if(this->windows_.begin(), this->windows_.end(),
                                   [](const auto &win) {
                                       return win.type_ == WindowType::Main;
                                   });

    if (mainWindow == this->windows_.end())
    {
        this->windows_.emplace_back(WindowDescriptor{
            .type_ = WindowType::Main,
            .geometry_ = {-1, -1, -1, -1},
            .tabs_ =
                {
                    TabDescriptor{
                        .selected_ = true,
                        .rootNode_ = SplitNodeDescriptor{{
                            .type_ = "twitch",
                            .channelName_ = name,
                        }},
                    },
                },
        });
        return;
    }

    TabDescriptor *bestTab = nullptr;
    // The tab score is calculated as follows:
    // +2 for every split
    // +1 if the desired split has filters
    // Thus lower is better and having one split of a channel is preferred over multiple
    size_t bestTabScore = std::numeric_limits<size_t>::max();

    for (auto &tab : mainWindow->tabs_)
    {
        tab.selected_ = false;

        if (!tab.rootNode_)
        {
            continue;
        }

        // recursive visitor
        struct Visitor {
            const QString &spec;
            size_t score = 0;
            bool hasChannel = false;

            void operator()(const SplitNodeDescriptor &split)
            {
                this->score += 2;
                if (split.channelName_ == this->spec)
                {
                    this->hasChannel = true;
                    if (!split.filters_.empty())
                    {
                        this->score += 1;
                    }
                }
            }

            void operator()(const ContainerNodeDescriptor &container)
            {
                for (const auto &item : container.items_)
                {
                    std::visit(*this, item);
                }
            }
        } visitor{name};

        std::visit(visitor, *tab.rootNode_);

        if (visitor.hasChannel && visitor.score < bestTabScore)
        {
            bestTab = &tab;
            bestTabScore = visitor.score;
        }
    }

    if (bestTab)
    {
        bestTab->selected_ = true;
        return;
    }

    TabDescriptor tab{
        .selected_ = true,
        .rootNode_ = SplitNodeDescriptor{{
            .type_ = "twitch",
            .channelName_ = name,
        }},
    };
    mainWindow->tabs_.emplace_back(tab);
}

}  // namespace chatterino
