#pragma once

#include "twitch-eventsub-ws/payloads/subscription.hpp"

#include <boost/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace chatterino::eventsub::lib::payload::channel_chat_notification::v1 {

struct Badge {
    std::string setID;
    std::string id;
    std::string info;
};

struct Cheermote {
    std::string prefix;
    int bits;
    int tier;
};

struct Emote {
    std::string id;
    std::string emoteSetID;
    std::string ownerID;
    std::vector<std::string> format;
};

struct Mention {
    std::string userID;
    std::string userName;
    std::string userLogin;
};

struct MessageFragment {
    std::string type;
    std::string text;
    std::optional<Cheermote> cheermote;
    std::optional<Emote> emote;
    std::optional<Mention> mention;
};

struct Subcription {
    static constexpr std::string_view TAG = "sub";

    std::string subTier;
    bool isPrime;
    int durationMonths;
};

struct Resubscription {
    static constexpr std::string_view TAG = "resub";

    int cumulativeMonths;
    int durationMonths;
    std::optional<int> streakMonths;
    std::string subTier;
    bool isPrime;
    bool isGift;
    bool gifterIsAnonymous;
    std::optional<std::string> gifterUserID;
    std::optional<std::string> gifterUserName;
    std::optional<std::string> gifterUserLogin;
};

struct GiftSubscription {
    static constexpr std::string_view TAG = "sub_gift";

    int durationMonths;
    std::optional<int> cumulativeTotal;
    std::optional<int> streakMonths;
    std::string recipientUserID;
    std::string recipientUserName;
    std::string recipientUserLogin;
    std::string subTier;
    std::optional<std::string> communityGiftID;
};

struct CommunityGiftSubscription {
    static constexpr std::string_view TAG = "community_sub_gift";

    std::string id;
    int total;
    std::string subTier;
    std::optional<int> cumulativeTotal;
};

struct GiftPaidUpgrade {
    static constexpr std::string_view TAG = "gift_paid_upgrade";

    bool gifterIsAnonymous;
    std::optional<std::string> gifterUserID;
    std::optional<std::string> gifterUserName;
    std::optional<std::string> gifterUserLogin;
};

struct PrimePaidUpgrade {
    static constexpr std::string_view TAG = "prime_paid_upgrade";

    std::string subTier;
};

struct Raid {
    static constexpr std::string_view TAG = "raid";

    std::string userID;
    std::string userName;
    std::string userLogin;
    int viewerCount;
    std::string profileImageURL;
};

struct Unraid {
    static constexpr std::string_view TAG = "unraid";
};

struct PayItForward {
    static constexpr std::string_view TAG = "pay_it_forward";

    bool gifterIsAnonymous;
    std::optional<std::string> gifterUserID;
    std::optional<std::string> gifterUserName;
    std::optional<std::string> gifterUserLogin;
};

struct Announcement {
    static constexpr std::string_view TAG = "announcement";

    std::string color;
};

struct CharityDonationAmount {
    static constexpr std::string_view TAG = "charity_donation_amount";

    int value;
    int decimalPlaces;
    std::string currency;
};

struct CharityDonation {
    static constexpr std::string_view TAG = "charity_donation";

    std::string charityName;
    CharityDonationAmount amount;
};

struct BitsBadgeTier {
    static constexpr std::string_view TAG = "bits_badge_tier";

    int tier;
};

struct Message {
    std::string text;
    std::vector<MessageFragment> fragments;
};

struct Event {
    std::string broadcasterUserID;
    std::string broadcasterUserLogin;
    std::string broadcasterUserName;
    std::string chatterUserID;
    std::string chatterUserLogin;
    std::string chatterUserName;
    bool chatterIsAnonymous;
    std::string color;
    std::vector<Badge> badges;
    std::string systemMessage;
    std::string messageID;
    Message message;
    /// json_tag=notice_type
    std::variant<Subcription,                //
                 Resubscription,             //
                 GiftSubscription,           //
                 CommunityGiftSubscription,  //
                 GiftPaidUpgrade,            //
                 PrimePaidUpgrade,           //
                 Raid,                       //
                 Unraid,                     //
                 PayItForward,               //
                 Announcement,               //
                 CharityDonation,            //
                 BitsBadgeTier,              //
                 std::string                 //
                 >
        inner;
};

struct Payload {
    const subscription::Subscription subscription;

    const Event event;
};

#include "twitch-eventsub-ws/payloads/channel-chat-notification-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_chat_notification::v1
