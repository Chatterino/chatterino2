// SPDX-FileCopyrightText: 2020 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/WindowDescriptors.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "util/QMagicEnum.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"
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

QList<QUuid> loadFilters(const QJsonValue &val)
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

QJsonArray encodeFilters(std::span<const QUuid> filters)
{
    QJsonArray array;
    for (const auto &f : filters)
    {
        array.append(f.toString(QUuid::WithoutBraces));
    }
    return array;
}

NodeDescriptor buildDescriptorRecursively(const SplitContainer::Node &node)
{
    using Type = SplitContainer::Node::Type;
    switch (node.getType())
    {
        case Type::Split: {
            SplitNodeDescriptor descriptor(
                SplitDescriptor::fromSplit(*node.getSplit()));
            descriptor.flexH_ = node.getHorizontalFlex();
            descriptor.flexV_ = node.getVerticalFlex();
            return descriptor;
        }
        case Type::HorizontalContainer:
        case Type::VerticalContainer: {
            ContainerNodeDescriptor descriptor{
                .flexH_ = node.getHorizontalFlex(),
                .flexV_ = node.getVerticalFlex(),
                .vertical_ = node.getType() == Type::VerticalContainer,
            };

            for (const auto &n : node.getChildren())
            {
                descriptor.items_.emplace_back(buildDescriptorRecursively(*n));
            }
            return descriptor;
        }
        case Type::EmptyRoot:
            return ContainerNodeDescriptor{
                .flexH_ = node.getHorizontalFlex(),
                .flexV_ = node.getVerticalFlex(),
            };
    }

    return ContainerNodeDescriptor{};
}

}  // namespace

SplitDescriptor SplitDescriptor::loadFromJSON(const QJsonObject &root)
{
    SplitDescriptor descriptor;

    auto data = root["data"].toObject();
    descriptor.type_ = data.value("type").toString();
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
    return descriptor;
}

SplitDescriptor SplitDescriptor::fromSplit(const Split &split)
{
    SplitDescriptor descriptor;
    descriptor.type_ =
        qmagicenum::enumNameString(split.getIndirectChannel().getType());
    descriptor.channelName_ = split.getChannel()->getName();
    descriptor.filters_ = split.getFilters();
    descriptor.moderationMode_ = split.getModerationMode();
    return descriptor;
}

void SplitDescriptor::appendJson(QJsonObject &root) const
{
    root.insert("moderationMode", this->moderationMode_);
    QJsonObject data{
        {"type", this->type_},
    };
    if (!this->channelName_.isEmpty())
    {
        data["name"] = this->channelName_;
    }
    root.insert("data", data);

    if (!this->filters_.empty())
    {
        root.insert("filters", encodeFilters(this->filters_));
    }

    if (this->spellCheckOverride)
    {
        root["checkSpelling"] = *this->spellCheckOverride;
    }
}

IndirectChannel SplitDescriptor::decodeChannel() const
{
    auto type = qmagicenum::enumCast<Channel::Type>(this->type_);
    if (!type)
    {
        return Channel::getEmpty();
    }

    switch (*type)
    {
        case Channel::Type::Twitch:
            return getApp()->getTwitch()->getOrAddChannel(this->channelName_);
        case Channel::Type::TwitchMentions:
            return getApp()->getTwitch()->getMentionsChannel();
        case Channel::Type::TwitchWatching:
            return getApp()->getTwitch()->getWatchingChannel();
        case Channel::Type::TwitchWhispers:
            return getApp()->getTwitch()->getWhispersChannel();
        case Channel::Type::TwitchLive:
            return getApp()->getTwitch()->getLiveChannel();
        case Channel::Type::TwitchAutomod:
            return getApp()->getTwitch()->getAutomodChannel();
        case Channel::Type::Misc:
            return getApp()->getTwitch()->getChannelOrEmpty(this->channelName_);

        case Channel::Type::None:
        case Channel::Type::Direct:
        case Channel::Type::TwitchEnd:
            break;  // FIXME: Remove these (#5703)
    }

    return Channel::getEmpty();
}

void SplitDescriptor::applyTo(Split &split) const
{
    split.setChannel(this->decodeChannel());
    split.setModerationMode(this->moderationMode_);
    split.setFilters(this->filters_);
    split.setCheckSpellingOverride(this->spellCheckOverride);
}

SplitNodeDescriptor::SplitNodeDescriptor(SplitDescriptor descriptor)
    : SplitDescriptor(std::move(descriptor))
{
}

SplitNodeDescriptor SplitNodeDescriptor::loadFromJSON(const QJsonObject &root)
{
    SplitNodeDescriptor descriptor(SplitDescriptor::loadFromJSON(root));
    descriptor.flexH_ = root["flexh"].toDouble(1.0);
    descriptor.flexV_ = root["flexv"].toDouble(1.0);
    return descriptor;
}

void SplitNodeDescriptor::appendJson(QJsonObject &root) const
{
    SplitDescriptor::appendJson(root);
    root.insert("flexh", this->flexH_);
    root.insert("flexv", this->flexV_);
    root.insert("type", "split");
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

void ContainerNodeDescriptor::appendJson(QJsonObject &root) const
{
    root.insert("flexh", this->flexH_);
    root.insert("flexv", this->flexV_);
    if (this->vertical_)
    {
        root.insert("type", "vertical");
    }
    else
    {
        root.insert("type", "horizontal");
    }

    QJsonArray items;
    for (const auto &item : this->items_)
    {
        QJsonObject obj;
        std::visit(
            [&](const auto &it) {
                it.appendJson(obj);
            },
            item);
        items.append(obj);
    }
    root.insert("items", items);
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

TabDescriptor TabDescriptor::fromRootContainer(const SplitContainer &container,
                                               bool isSelected)
{
    TabDescriptor descriptor;
    if (container.getTab()->hasCustomTitle())
    {
        descriptor.customTitle_ = container.getTab()->getCustomTitle();
    }
    descriptor.selected_ = isSelected;
    descriptor.highlightsEnabled_ = container.getTab()->hasHighlightsEnabled();

    // splits
    if (container.getBaseNode()->getType() !=
        SplitContainer::Node::Type::EmptyRoot)
    {
        descriptor.rootNode_ =
            buildDescriptorRecursively(*container.getBaseNode());
    }
    return descriptor;
}

void TabDescriptor::appendJson(QJsonObject &root) const
{
    if (!this->customTitle_.isEmpty())
    {
        root.insert("title", this->customTitle_);
    }
    root.insert("selected", this->selected_);
    root.insert("highlightsEnabled", this->highlightsEnabled_);
    if (this->rootNode_)
    {
        QJsonObject splits;
        std::visit(
            [&](const auto &it) {
                it.appendJson(splits);
            },
            *this->rootNode_);
        root.insert("splits2", splits);
    }
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
