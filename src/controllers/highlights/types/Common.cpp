#include "controllers/highlights/types/Common.hpp"

#include "controllers/highlights/types/All.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "util/Variant.hpp"

#include <QIcon>
#include <QUrl>
#include <QUuid>

#include <cassert>

namespace chatterino::highlights {

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
concept HasDynamicName = requires(T a) {
    { a.getName() } -> std::same_as<QString>;
};

template <typename T>
concept HasDynamicIcon = requires(T a) {
    { a.getType() } -> std::same_as<QIcon>;
};

bool matchesType(const rapidjson::Value &obj, QStringView expectedType)
{
    assert(obj.IsObject());

    if (!obj.IsObject())
    {
        return false;
    }

    auto member = obj.FindMember("type");
    if (member == obj.MemberEnd())
    {
        return false;
    }

    QString actualType =
        pajlada::Deserialize<QString>::get(member->value, nullptr);

    return actualType == expectedType;
}

bool matchesID(const rapidjson::Value &obj, QStringView expectedID)
{
    assert(obj.IsObject());

    if (!obj.IsObject())
    {
        return false;
    }

    auto member = obj.FindMember("id");
    if (member == obj.MemberEnd())
    {
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
                          [](const HasDynamicDefaultName auto &h) {
                              if (h.name.isEmpty())
                              {
                                  return h.getDefaultName();
                              }
                              return h.name;
                          },
                          [](const auto &h) {
                              if (h.name.isEmpty())
                              {
                                  using ActualType = std::decay_t<decltype(h)>;
                                  return ActualType::DEFAULT_NAME.toString();
                              }
                              return h.name;
                          },
                      },
                      h);
}

bool isEnabled(const AllHighlights &h)
{
    return std::visit(
        [](auto &&h) {
            using ActualType = std::decay_t<decltype(h)>;
            return h.enabled.value_or(ActualType::ENABLED_BY_DEFAULT);
        },
        h);
}

QUrl getSoundURL(const AllHighlights &h)
{
    return std::visit(
        [](auto &&h) {
            return h.outcome.customSoundURL;
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
    return std::visit(
        [](auto &&h) {
            using ActualType = std::decay_t<decltype(h)>;
            return h.outcome.playSound.value_or(ActualType::PLAY_SOUND_DEFAULT);
        },
        h);
}

bool willPlayCustomSound(const AllHighlights &h)
{
    return std::visit(
        [](auto &&h) {
            using ActualType = std::decay_t<decltype(h)>;
            return h.outcome.playSound.value_or(
                       ActualType::PLAY_SOUND_DEFAULT) &&
                   !h.outcome.customSoundURL.isEmpty();
        },
        h);
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

}  // namespace chatterino::highlights
