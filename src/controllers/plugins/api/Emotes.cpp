// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/api/Emotes.hpp"

#    include "controllers/plugins/SolTypes.hpp"
#    include "messages/Emote.hpp"

#    include <sol/sol.hpp>

namespace {

using namespace chatterino;
using namespace chatterino::lua;

EmotePtr newUncached(const sol::table &tbl)
{
    auto homePage = tbl.get_or("home_page", QString{});
    if (!homePage.isEmpty() &&
        !(homePage.startsWith(u"https://") || homePage.startsWith(u"http://")))
    {
        throw std::runtime_error("`home_page` must be an http(s) link");
    }

    return std::make_shared<const Emote>(Emote{
        .name = {requiredGet<QString>(tbl, "name")},
        .images = requiredGet<ImageSet>(tbl, "images"),
        .tooltip = {requiredGet<QString>(tbl, "tooltip")},
        .homePage = {std::move(homePage)},
        .zeroWidth = tbl.get_or("zero_width", false),
        .id = {tbl.get_or("id", QString{})},
        .author = {tbl.get_or("author", QString{})},
        .baseName = tbl.get<std::optional<QString>>("base_name")
                        .transform([](auto name) {
                            return EmoteName{std::move(name)};
                        }),
    });
}

}  // namespace

namespace chatterino::lua::api::emotes {

void createUserTypes(sol::table &c2)
{
    c2.new_usertype<Emote>(
        "Emote", sol::no_constructor,  //
        "new_uncached", &newUncached,  //
        "name", sol::property([](const Emote &emote) {
            return emote.name.string;
        }),
        "images", sol::property([](const Emote &emote) {
            return emote.images;
        }),
        "tooltip", sol::property([](const Emote &emote) {
            return emote.tooltip.string;
        }),
        "home_page", sol::property([](const Emote &emote) {
            return emote.homePage.string;
        }),
        "zero_width", sol::property([](const Emote &emote) {
            return emote.zeroWidth;
        }),
        "id", sol::property([](const Emote &emote) {
            return emote.id.string;
        }),
        "author", sol::property([](const Emote &emote) {
            return emote.author.string;
        }),
        "base_name", sol::property([](const Emote &emote) {
            return emote.baseName.transform([](const auto &it) {
                return it.string;
            });
        })  //
    );
}

}  // namespace chatterino::lua::api::emotes

#endif
