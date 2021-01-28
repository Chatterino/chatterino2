#include "common/WindowDescriptors.hpp"

#include "common/QLogging.hpp"
#include "widgets/Window.hpp"

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

WindowLayout WindowLayout::loadFromFile(const QString &path)
{
    WindowLayout layout;

    bool hasSetAMainWindow = false;

    // "deserialize"
    for (const QJsonValue &window_val : loadWindowArray(path))
    {
        QJsonObject window_obj = window_val.toObject();

        WindowDescriptor window;

        // Load window type
        QString type_val = window_obj.value("type").toString();
        auto type = type_val == "main" ? WindowType::Main : WindowType::Popup;

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
        if (window_obj.value("state") == "minimized")
        {
            window.state_ = WindowDescriptor::State::Minimized;
        }
        else if (window_obj.value("state") == "maximized")
        {
            window.state_ = WindowDescriptor::State::Maximized;
        }

        // Load window geometry
        {
            int x = window_obj.value("x").toInt(-1);
            int y = window_obj.value("y").toInt(-1);
            int width = window_obj.value("width").toInt(-1);
            int height = window_obj.value("height").toInt(-1);

            window.geometry_ = QRect(x, y, width, height);
        }

        bool hasSetASelectedTab = false;

        // Load window tabs
        QJsonArray tabs = window_obj.value("tabs").toArray();
        for (QJsonValue tab_val : tabs)
        {
            TabDescriptor tab;

            QJsonObject tab_obj = tab_val.toObject();

            // Load tab custom title
            QJsonValue title_val = tab_obj.value("title");
            if (title_val.isString())
            {
                tab.customTitle_ = title_val.toString();
            }

            // Load tab selected state
            tab.selected_ = tab_obj.value("selected").toBool(false);

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

            // Load tab "highlightsEnabled" state
            tab.highlightsEnabled_ =
                tab_obj.value("highlightsEnabled").toBool(true);

            QJsonObject splitRoot = tab_obj.value("splits2").toObject();

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
                    tab.rootNode_ =
                        loadNodes<ContainerNodeDescriptor>(splitRoot);
                }
            }

            window.tabs_.emplace_back(std::move(tab));
        }

        // Load emote popup position
        QJsonObject emote_popup_obj = window_obj.value("emotePopup").toObject();
        layout.emotePopupPos_ = QPoint(emote_popup_obj.value("x").toInt(),
                                       emote_popup_obj.value("y").toInt());

        layout.windows_.emplace_back(std::move(window));
    }

    return layout;
}

}  // namespace chatterino
