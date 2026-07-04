#include "controllers/plugins/api/WindowManager.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/api/ChannelRef.hpp"
#    include "controllers/plugins/api/Message.hpp"
#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/SignalCallback.hpp"
#    include "controllers/plugins/SolTypes.hpp"  // IWYU pragma: keep
#    include "messages/layouts/MessageLayout.hpp"
#    include "messages/layouts/MessageLayoutElement.hpp"
#    include "singletons/WindowManager.hpp"
#    include "util/WeakPtrHelpers.hpp"
#    include "widgets/helper/ChannelView.hpp"
#    include "widgets/Notebook.hpp"
#    include "widgets/splits/Split.hpp"
#    include "widgets/Window.hpp"

namespace {

using namespace chatterino;

/// Create a table with all items from `items` wrapped in a `QPointer`.
sol::table qPointerWrapped(const auto &items, sol::this_state state)
{
    auto tbl =
        sol::state_view(state).create_table(static_cast<int>(items.size()), 0);

    for (size_t idx = 0; idx < items.size(); idx++)
    {
        tbl[static_cast<int>(idx + 1)] = QPointer(items[idx]);
    }
    return tbl;
}

/// Wraps a `std::weak_ptr<SplitContainer::Node>` and adds convenience functions
/// and an `operator==`.
///
/// In Lua, all nodes have this type instead of a raw `weak_ptr`. In Chatterino,
/// nodes are `std::shared_ptr`s and often nodes are passed around as raw
/// pointers.
struct SplitContainerNodeWrap {
    SplitContainerNodeWrap(std::weak_ptr<SplitContainer::Node> ptr)
        : ptr(std::move(ptr))
    {
    }

    sol::table children(sol::this_state state) const
    {
        const auto &nodes = this->strong()->getChildren();
        auto tbl = sol::state_view(state).create_table(
            static_cast<int>(nodes.size()), 0);

        for (size_t idx = 0; idx < nodes.size(); idx++)
        {
            tbl[static_cast<int>(idx + 1)] = SplitContainerNodeWrap{nodes[idx]};
        }
        return tbl;
    }

    bool isValid() const
    {
        return !this->ptr.expired();
    }

    std::shared_ptr<SplitContainer::Node> strong() const
    {
        auto locked = this->ptr.lock();
        if (locked)
        {
            return locked;
        }
        throw std::runtime_error("Split container node does not exist anymore");
    }

    bool operator==(const SplitContainerNodeWrap &other) const
    {
        return weakOwnerEquals(this->ptr, other.ptr);
    }

    static std::optional<SplitContainerNodeWrap> fromPtr(
        SplitContainer::Node *ptr)
    {
        if (ptr)
        {
            return SplitContainerNodeWrap(ptr->weak_from_this());
        }
        return std::nullopt;
    }

private:
    std::weak_ptr<SplitContainer::Node> ptr;
};

}  // namespace

namespace chatterino::lua::api::windowmanager {

void createUserTypes(sol::table &c2)
{
    c2.new_usertype<Split>("Split", sol::no_constructor,  //
                           "channel",
                           sol::readonly_property([](const Split &self) {
                               return ChannelRef(self.getChannel());
                           }));

    c2.new_usertype<SplitContainerNodeWrap>(
        "SplitContainerNode", sol::no_constructor,     //
        "is_valid", &SplitContainerNodeWrap::isValid,  //
        "type", sol::readonly_property([](const SplitContainerNodeWrap &self) {
            return self.strong()->getType();
        }),
        "split", sol::readonly_property([](const SplitContainerNodeWrap &self) {
            return QPointer(self.strong()->getSplit());
        }),
        "parent",
        sol::readonly_property([](const SplitContainerNodeWrap &self) {
            return SplitContainerNodeWrap::fromPtr(self.strong()->getParent());
        }),
        "horizontal_flex",
        sol::readonly_property([](const SplitContainerNodeWrap &self) {
            return self.strong()->getHorizontalFlex();
        }),
        "vertical_flex",
        sol::readonly_property([](const SplitContainerNodeWrap &self) {
            return self.strong()->getVerticalFlex();
        }),
        "children", &SplitContainerNodeWrap::children);

    // Wrapped in a QPointer
    c2.new_usertype<SplitContainer>(
        "SplitContainer", sol::no_constructor,  //
        "selected_split",
        sol::property(
            [](const SplitContainer &self) {
                return QPointer(self.getSelectedSplit());
            },
            [](SplitContainer &self, Split &split) {
                self.setSelected(&split);
            }),
        "base_node", sol::readonly_property([](SplitContainer &self) {
            return SplitContainerNodeWrap::fromPtr(self.getBaseNode());
        }),  //
        "splits", [](SplitContainer &self, sol::this_state state) {
            return qPointerWrapped(self.getSplits(), state);
        });

    // Wrapped in a QPointer
    c2.new_usertype<SplitNotebook>(
        "SplitNotebook", sol::no_constructor,  //
        "selected_page", sol::readonly_property([](SplitNotebook &self) {
            return QPointer(self.getSelectedPage());
        }),
        "page_count", sol::readonly_property(&SplitNotebook::getPageCount),
        "page_at", [](const SplitNotebook &self, int index) {
            if (index < 0 || index >= self.getPageCount())
            {
                return QPointer<SplitContainer>(nullptr);
            }
            return QPointer(
                dynamic_cast<SplitContainer *>(self.getPageAt(index)));
        });

    // Wrapped in a QPointer
    c2.new_usertype<Window>("Window", sol::no_constructor,  //
                            "notebook",
                            sol::readonly_property([](Window &self) {
                                return QPointer(&self.getNotebook());
                            }),
                            "type", sol::readonly_property([](Window &self) {
                                return self.getType();
                            }));

    c2.new_usertype<WindowManager>(
        "WindowManager", sol::no_constructor,  //
        "main_window", sol::readonly_property([](WindowManager &self) {
            return QPointer(&self.getMainWindow());
        }),
        "last_selected_window",
        sol::readonly_property([](const WindowManager &self) {
            return QPointer(self.getLastSelectedWindow());
        }),
        "all",
        [](const WindowManager &self, sol::this_state state) {
            return qPointerWrapped(self.windows(), state);
        },
        "on_channelview_context_menu_requested",
        [](WindowManager &self, ThisPluginState state,
           sol::main_protected_function cb) {
            return state.plugin()->connections.managedConnect(
                self.channelViewContextMenuRequested,
                [cb = SignalCallback(state.plugin()->weakRef(), std::move(cb))](
                    const ChannelView &view, const MessageLayout &layout,
                    const MessageLayoutElement *element, QMenu &menu) {
                    auto pluginRef = cb.owner().strong();
                    if (!pluginRef)
                    {
                        return;
                    }

                    std::optional<message::ElementRef> ref;
                    auto msg = std::const_pointer_cast<Message>(
                        layout.getMessagePtr());
                    if (element)
                    {
                        auto *el = &element->getCreator();
                        for (size_t i = 0; i < msg->elements.size(); ++i)
                        {
                            if (msg->elements[i].get() == el)
                            {
                                ref.emplace(msg, i);
                                break;
                            }
                        }
                    }
                    std::optional<ChannelRef> maybeChan;
                    if (auto underlyingChan = view.underlyingChannel())
                    {
                        maybeChan.emplace(std::move(underlyingChan));
                    }

                    auto tbl = pluginRef.plugin()->state().create_table_with(
                        "split", QPointer(view.findParentSplit()),  //
                        "channel", std::move(maybeChan),            //
                        "menu", QPointer(&menu),                    //
                        "message", std::move(msg),                  //
                        "message_element", std::move(ref)           //
                    );
                    cb(std::move(tbl));
                });
        });
}

}  // namespace chatterino::lua::api::windowmanager

#endif
