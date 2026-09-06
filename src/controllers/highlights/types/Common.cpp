// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/Common.hpp"

#include "common/QLogging.hpp"
#include "controllers/highlights/types/All.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "util/Variant.hpp"

#include <QIcon>
#include <QUrl>
#include <QUuid>

#include <cassert>

namespace chatterino::highlights {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoHighlights;

}  // namespace

template <typename T>
concept HasDynamicID = requires(T a) {
    { a.getID() } -> std::same_as<QStringView>;
};

template <typename T>
concept HasDynamicDefaultName = requires(T a) {
    // TODO: QStringView
    { a.getDefaultName() } -> std::same_as<QString>;
};

template <typename T>
concept HasCustomizableName = requires(T a) {
    { a.name } -> std::convertible_to<QString>;
};

template <typename T>
concept HasDynamicAndCustomizableName =
    HasDynamicDefaultName<T> && HasCustomizableName<T>;

template <typename T>
concept SupportsErrors = requires(T a) {
    { a.getError() } -> std::same_as<QString>;
};

template <typename T>
concept HasDefaultSound = requires {
    { T::SOUND_DEFAULT } -> std::convertible_to<QStringView>;
};

static_assert(HasDefaultSound<YourUsernameHighlight>);

bool matchesType(const rapidjson::Value &object, QStringView expectedType)
{
    assert(object.IsObject());

    if (!object.IsObject())
    {
        qCWarning(LOG) << "Error in matchesType, given object is not an object:"
                       << rj::stringify(object);
        return false;
    }

    auto member = object.FindMember("type");
    if (member == object.MemberEnd())
    {
        return false;
    }

    QString actualType =
        pajlada::Deserialize<QString>::get(member->value, nullptr);

    return actualType == expectedType;
}

bool matchesID(const rapidjson::Value &object, QStringView expectedID)
{
    assert(object.IsObject());

    if (!object.IsObject())
    {
        qCWarning(LOG) << "Error in matchesId, given object is not an object:"
                       << rj::stringify(object);
        return false;
    }

    auto member = object.FindMember("id");
    if (member == object.MemberEnd())
    {
        // All highlights must contain an id
        qCWarning(LOG)
            << "Error in matchesId, object does not contain the id key:"
            << rj::stringify(object);
        return false;
    }

    QString actualID =
        pajlada::Deserialize<QString>::get(member->value, nullptr);

    return actualID == expectedID;
}

QString generateID()
{
    return QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
}

QStringView getID(const AllHighlights &h)
{
    return std::visit(variant::Overloaded{
                          [](const HasDynamicID auto &h) {
                              return h.getID();
                          },
                          [](const auto &h) {
                              using ActualType = std::decay_t<decltype(h)>;
                              return ActualType::ID;
                          },
                      },
                      h);
}

QString getDefaultName(const AllHighlights &h)
{
    return std::visit(variant::Overloaded{
                          [](const HasDynamicDefaultName auto &h) {
                              return h.getDefaultName();
                          },
                          [](const auto &h) {
                              using ActualType = std::decay_t<decltype(h)>;
                              return ActualType::DEFAULT_NAME.toString();
                          },
                      },
                      h);
}

QString getName(const AllHighlights &h)
{
    return std::visit(variant::Overloaded{
                          [](const HasDynamicAndCustomizableName auto &h) {
                              if (h.name.isEmpty())
                              {
                                  return h.getDefaultName();
                              }
                              return h.name;
                          },
                          [](const HasCustomizableName auto &h) {
                              if (h.name.isEmpty())
                              {
                                  using ActualType = std::decay_t<decltype(h)>;
                                  return ActualType::DEFAULT_NAME.toString();
                              }
                              return h.name;
                          },
                          [](const auto &h) {
                              using ActualType = std::decay_t<decltype(h)>;
                              return ActualType::DEFAULT_NAME.toString();
                          },
                      },
                      h);
}

bool isEnabled(const AllHighlights &h)
{
    return std::visit(
        variant::Overloaded{
            [](const InvalidHighlight & /*h*/) {
                return false;
            },
            [](const auto &h) {
                using ActualType = std::decay_t<decltype(h)>;
                return h.enabled.value_or(ActualType::ENABLED_BY_DEFAULT);
            },
        },
        h);
}

QString getSound(const AllHighlights &h)
{
    return std::visit(variant::Overloaded{
                          [](const HasDefaultSound auto &h) {
                              using ActualType = std::decay_t<decltype(h)>;

                              if (h.outcome.sound.isNull())
                              {
                                  return ActualType::SOUND_DEFAULT.toString();
                              }

                              return h.outcome.sound;
                          },
                          [](auto &&h) {
                              return h.outcome.sound;
                          },
                      },
                      h);
}

QStringView getDefaultSound(const AllHighlights &h)
{
    return std::visit(variant::Overloaded{
                          [](const HasDefaultSound auto &h) {
                              // static_assert(false);
                              using ActualType = std::decay_t<decltype(h)>;

                              return ActualType::SOUND_DEFAULT;
                          },
                          [](auto &&h) {
                              return QStringView{};
                          },
                      },
                      h);
}

QUrl getSoundURL(const AllHighlights &h)
{
    return std::visit(
        [](auto &&h) {
            return h.outcome.soundURL;
        },
        h);
}

bool shouldShowInMentions(const AllHighlights &h)
{
    return std::visit(
        [](auto &&h) {
            using ActualType = std::decay_t<decltype(h)>;
            return h.outcome.showInMentions.value_or(
                ActualType::SHOW_IN_MENTIONS_DEFAULT);
        },
        h);
}

bool shouldAlert(const AllHighlights &h)
{
    return std::visit(
        [](auto &&h) {
            using ActualType = std::decay_t<decltype(h)>;
            return h.outcome.alert.value_or(ActualType::ALERT_DEFAULT);
        },
        h);
}

bool shouldPlaySound(const AllHighlights &h)
{
    return false;
    /* TODO
    return std::visit(
        [](auto &&h) {
            using ActualType = std::decay_t<decltype(h)>;
            return h.outcome.playSound.value_or(ActualType::PLAY_SOUND_DEFAULT);
        },
        h);
        */
}

bool willPlayCustomSound(const AllHighlights &h)
{
    return false;
    /* TODO
    return std::visit(
        [](auto &&h) {
            using ActualType = std::decay_t<decltype(h)>;
            return h.outcome.playSound.value_or(
                       ActualType::PLAY_SOUND_DEFAULT) &&
                   !h.outcome.customSoundURL.isEmpty();
        },
        h);
        */
}

QIcon getIcon(const AllHighlights &h)
{
    return std::visit(
        [](auto &&h) {
            using ActualType = std::decay_t<decltype(h)>;
            return QIcon{ActualType::ICON_RESOURCE.toString()};
        },
        h);
}

std::shared_ptr<QColor> getBackgroundColor(const AllHighlights &h)
{
    auto c = std::visit(
        [](auto &&h) {
            return h.outcome.getBackgroundColor();
        },
        h);

    assert(c);

    return c;
}

QString getError(const AllHighlights &h)
{
    return std::visit(variant::Overloaded{
                          [](const SupportsErrors auto &h) {
                              return h.getError();
                          },
                          [](const auto & /*h*/) {
                              return QString{};
                          },
                      },
                      h);
}

}  // namespace chatterino::highlights
