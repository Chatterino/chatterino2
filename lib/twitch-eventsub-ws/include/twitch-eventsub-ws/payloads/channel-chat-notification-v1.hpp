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
    std::string subTier;
    bool isPrime;
    int durationMonths;
};

struct Resubscription {
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
    std::string id;
    int total;
    std::string subTier;
    std::optional<int> cumulativeTotal;
};

struct GiftPaidUpgrade {
    bool gifterIsAnonymous;
    std::optional<std::string> gifterUserID;
    std::optional<std::string> gifterUserName;
    std::optional<std::string> gifterUserLogin;
};

struct PrimePaidUpgrade {
    std::string subTier;
};

struct Raid {
    std::string userID;
    std::string userName;
    std::string userLogin;
    int viewerCount;
    std::string profileImageURL;
};

struct Unraid {
};

struct PayItForward {
    bool gifterIsAnonymous;
    std::optional<std::string> gifterUserID;
    std::optional<std::string> gifterUserName;
    std::optional<std::string> gifterUserLogin;
};

struct Announcement {
    std::string color;
};

struct CharityDonationAmount {
    int value;
    int decimalPlaces;
    std::string currency;
};

struct CharityDonation {
    std::string charityName;
    CharityDonationAmount amount;
};

struct BitsBadgeTier {
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
    std::string noticeType;
    std::optional<Subcription> sub;
    std::optional<Resubscription> resub;
    std::optional<GiftSubscription> subGift;
    std::optional<CommunityGiftSubscription> communitySubGift;
    std::optional<GiftPaidUpgrade> giftPaidUpgrade;
    std::optional<PrimePaidUpgrade> primePaidUpgrade;
    std::optional<Raid> raid;
    std::optional<Unraid> unraid;
    std::optional<PayItForward> payItForward;
    std::optional<Announcement> announcement;
    std::optional<CharityDonation> charityDonation;
    std::optional<BitsBadgeTier> bitsBadgeTier;
};

struct Payload {
    const subscription::Subscription subscription;

    const Event event;
};

#include "twitch-eventsub-ws/payloads/channel-chat-notification-v1.inc"

}  // namespace chatterino::eventsub::lib::payload::channel_chat_notification::v1
