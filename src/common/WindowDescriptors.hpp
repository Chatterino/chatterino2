#pragma once

#include <QString>

#include <optional>
#include <variant>

namespace chatterino {

/**
 * A WindowLayout contains one or more windows.
 * Only one of those windows can be the main window
 *
 * Each window contains a list of tabs.
 * Only one of those tabs can be marked as selected.
 *
 * Each tab contains a root node.
 * The root node is either a:
 *  - Split Node (for single-split tabs), or
 *  - Container Node (for multi-split tabs).
 *    This container node would then contain a list of nodes on its own, which could be split nodes or further container nodes
 **/

// from widgets/Window.hpp
enum class WindowType;

struct SplitDescriptor {
    // twitch or mentions or watching or whispers or irc
    QString type_;

    // Twitch Channel name or IRC channel name
    QString channelName_;

    // IRC server
    int server_{-1};

    // Whether "Moderation Mode" (the sword icon) is enabled in this split or not
    bool moderationMode_{false};

    QList<QUuid> filters_;

    static void loadFromJSON(SplitDescriptor &descriptor,
                             const QJsonObject &root, const QJsonObject &data);
};

struct SplitNodeDescriptor : SplitDescriptor {
    qreal flexH_ = 1;
    qreal flexV_ = 1;
};

struct ContainerNodeDescriptor;

using NodeDescriptor =
    std::variant<ContainerNodeDescriptor, SplitNodeDescriptor>;

struct ContainerNodeDescriptor {
    qreal flexH_ = 1;
    qreal flexV_ = 1;

    bool vertical_ = false;

    std::vector<NodeDescriptor> items_;
};

struct TabDescriptor {
    QString customTitle_;
    bool selected_{false};
    bool highlightsEnabled_{true};

    std::optional<NodeDescriptor> rootNode_;
};

struct WindowDescriptor {
    enum class State {
        None,
        Minimized,
        Maximized,
    };

    WindowType type_;
    State state_ = State::None;

    QRect geometry_;

    std::vector<TabDescriptor> tabs_;
};

class WindowLayout
{
public:
    static WindowLayout loadFromFile(const QString &path);

    // A complete window layout has a single emote popup position that is shared among all windows
    QPoint emotePopupPos_;

    std::vector<WindowDescriptor> windows_;
};

}  // namespace chatterino
