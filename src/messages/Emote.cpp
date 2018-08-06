#include "Emote.hpp"

#include <unordered_map>

namespace chatterino {

bool operator==(const Emote &a, const Emote &b)
{
    return std::tie(a.homePage, a.name, a.tooltip, a.images) ==
           std::tie(b.homePage, b.name, b.tooltip, b.images);
}

bool operator!=(const Emote &a, const Emote &b)
{
    return !(a == b);
}

// EmotePtr Emote::create(const EmoteData2 &data)
//{
//}

// EmotePtr Emote::create(EmoteData2 &&data)
//{
//}

// Emote::Emote(EmoteData2 &&data)
//    : data_(data)
//{
//}
//
// Emote::Emote(const EmoteData2 &data)
//    : data_(data)
//{
//}
//
// const Url &Emote::getHomePage() const
//{
//    return this->data_.homePage;
//}
//
// const EmoteName &Emote::getName() const
//{
//    return this->data_.name;
//}
//
// const Tooltip &Emote::getTooltip() const
//{
//    return this->data_.tooltip;
//}
//
// const ImageSet &Emote::getImages() const
//{
//    return this->data_.images;
//}
//
// const QString &Emote::getCopyString() const
//{
//    return this->data_.name.string;
//}
//
// bool Emote::operator==(const Emote &other) const
//{
//    auto &a = this->data_;
//    auto &b = other.data_;
//
//    return std::tie(a.homePage, a.name, a.tooltip, a.images) ==
//           std::tie(b.homePage, b.name, b.tooltip, b.images);
//}
//
// bool Emote::operator!=(const Emote &other) const
//{
//    return !this->operator==(other);
//}

}  // namespace chatterino
