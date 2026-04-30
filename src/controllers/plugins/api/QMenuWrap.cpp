// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/plugins/api/QMenuWrap.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/SignalCallback.hpp"
#    include "controllers/plugins/SolTypes.hpp"

#    include <QMenu>

namespace {

QAction *findAction(const QMenu &menu, const QString &name)
{
    const auto actions = menu.actions();
    for (QAction *action : actions)
    {
        if (action->text() == name)
        {
            return action;
        }
    }
    return nullptr;
}

QAction *findAction(const QMenu &menu, int n)
{
    n -= 1;  // Take one-based indices

    const auto actions = menu.actions();
    if (n < 0 || n >= actions.size())
    {
        return nullptr;
    }
    return menu.actions().at(n);
}

QAction *findAction(const QMenu &menu, const std::variant<QString, int> &spec)
{
    return std::visit(
        [&](auto &&it) {
            return findAction(menu, it);
        },
        spec);
}

}  // namespace

namespace chatterino::lua::api::qmenu {

void createUserType(sol::table &c2)
{
    c2.new_usertype<QMenu>(
        "QMenu", sol::no_constructor,  //

        "add_action",
        [](QMenu &menu, const QString &name, ThisPluginState state,
           sol::main_protected_function cb) {
            menu.addAction(
                name, SignalCallback(state.plugin()->weakRef(), std::move(cb)));
        },
        "insert_action",
        [](QMenu &menu, const std::variant<QString, int> &before,
           const QString &name, ThisPluginState state,
           sol::main_protected_function cb) {
            auto *act = new QAction(name, &menu);
            QObject::connect(
                act, &QAction::triggered,
                SignalCallback(state.plugin()->weakRef(), std::move(cb)));
            menu.insertAction(findAction(menu, before), act);
        },

        "add_menu",
        [](QMenu &menu, const QString &title) {
            return QPointer(menu.addMenu(title));
        },
        "insert_menu",
        [](QMenu &menu, const std::variant<QString, int> &before,
           const QString &title) {
            return QPointer(menu.insertMenu(findAction(menu, before),
                                            new QMenu(title, &menu)));
        },

        "add_separator",
        [](QMenu &menu) {
            menu.addSeparator();
        },
        "insert_separator",
        [](QMenu &menu, const std::variant<QString, int> &before) {
            menu.insertSeparator(findAction(menu, before));
        }  //
    );
}

}  // namespace chatterino::lua::api::qmenu

#endif
