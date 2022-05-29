#include "common/WindowDescriptors.hpp"

#include "common/QLogging.hpp"
#include "widgets/Window.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace chatterino {

namespace {

    QJsonArray loadWindowArray(const QString &settingsPath)
    {
        QFile file(settingsPath);
        file.open(QIODevice::ReadOnly);
        QByteArray data = file.readAll();
        QJsonDocument document = QJsonDocument::fromJson(data);
        QJsonArray windows_arr = document.object().value("windows").toArray();
        return windows_arr;
    }

    template <typename T>
    T loadNodes(const QJsonObject &obj)
    {
        static_assert("loadNodes must be called with the SplitNodeDescriptor "
                      "or ContainerNodeDescriptor type");
    }

    template <>
    SplitNodeDescriptor loadNodes(const QJsonObject &root)
    {
        SplitNodeDescriptor descriptor;

        descriptor.flexH_ = root.value("flexh").toDouble(1.0);
        descriptor.flexV_ = root.value("flexv").toDouble(1.0);

        auto data = root.value("data").toObject();

        SplitDescriptor::loadFromJSON(descriptor, root, data);

        return descriptor;
    }

    template <>
    ContainerNodeDescriptor loadNodes(const QJsonObject &root)
    {
        ContainerNodeDescriptor descriptor;

        descriptor.flexH_ = root.value("flexh").toDouble(1.0);
        descriptor.flexV_ = root.value("flexv").toDouble(1.0);

        descriptor.vertical_ = root.value("type").toString() == "vertical";

        for (QJsonValue _val : root.value("items").toArray())
        {
            auto _obj = _val.toObject();

            auto _type = _obj.value("type");
            if (_type == "split")
            {
                descriptor.items_.emplace_back(
                    loadNodes<SplitNodeDescriptor>(_obj));
            }
            else
            {
                descriptor.items_.emplace_back(
                    loadNodes<ContainerNodeDescriptor>(_obj));
            }
        }

        return descriptor;
    }

    const QList<QUuid> loadFilters(QJsonValue val)
    {
        QList<QUuid> filterIds;

        if (!val.isUndefined())
        {
            const auto array = val.toArray();
            filterIds.reserve(array.size());
            for (const auto &id : array)
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
            tab.rootNode_ = loadNodes<SplitNodeDescriptor>(splitRoot);
        }
        else if (nodeType == "horizontal" || nodeType == "vertical")
        {
            tab.rootNode_ = loadNodes<ContainerNodeDescriptor>(splitRoot);
        }
    }

    return tab;
}

WindowLayout WindowLayout::loadFromFile(const QString &path)
{
    WindowLayout layout;

    bool hasSetAMainWindow = false;

    // "deserialize"
    for (const QJsonValue &windowVal : loadWindowArray(path))
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
        QJsonObject emote_popup_obj = windowObj.value("emotePopup").toObject();
        layout.emotePopupPos_ = QPoint(emote_popup_obj.value("x").toInt(),
                                       emote_popup_obj.value("y").toInt());

        layout.windows_.emplace_back(std::move(window));
    }

    return layout;
}

}  // namespace chatterino
