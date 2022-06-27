#include "controllers/highlights/HighlightController.hpp"

#include "common/QLogging.hpp"

namespace {

using namespace chatterino;

auto highlightPhraseCheck(const HighlightPhrase &highlight) -> HighlightCheck
{
    return HighlightCheck{
        [highlight](const auto &args, const auto &badges,
                    const auto &senderName, const auto &originalMessage,
                    const auto self) -> boost::optional<HighlightResult> {
            (void)args;        // unused
            (void)badges;      // unused
            (void)senderName;  // unused

            if (self)
            {
                // Phrase checks should ignore highlights from the user
                return boost::none;
            }

            if (!highlight.isMatch(originalMessage))
            {
                return boost::none;
            }

            boost::optional<QUrl> highlightSoundUrl;
            if (highlight.hasCustomSound())
            {
                highlightSoundUrl = highlight.getSoundUrl();
            }

            return HighlightResult{
                highlight.hasAlert(),       highlight.hasSound(),
                highlightSoundUrl,          highlight.getColor(),
                highlight.showInMentions(),
            };
        }};
}

void rebuildSubscriptionHighlights(Settings &settings,
                                   std::vector<HighlightCheck> &checks)
{
    if (settings.enableSubHighlight)
    {
        auto highlightSound = settings.enableSubHighlightSound.getValue();
        auto highlightAlert = settings.enableSubHighlightTaskbar.getValue();
        auto highlightSoundUrlValue =
            settings.whisperHighlightSoundUrl.getValue();
        boost::optional<QUrl> highlightSoundUrl;
        if (!highlightSoundUrlValue.isEmpty())
        {
            highlightSoundUrl = highlightSoundUrlValue;
        }

        // The custom sub highlight color is handled in ColorProvider

        checks.emplace_back(HighlightCheck{
            [=](const auto &args, const auto &badges, const auto &senderName,
                const auto &originalMessage,
                const auto self) -> boost::optional<HighlightResult> {
                (void)badges;           // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                (void)self;             // unused

                if (!args.isSubscriptionMessage)
                {
                    return boost::none;
                }

                auto highlightColor =
                    ColorProvider::instance().color(ColorType::Subscription);

                return HighlightResult{
                    highlightAlert,     // alert
                    highlightSound,     // playSound
                    highlightSoundUrl,  // customSoundUrl
                    highlightColor,     // color
                    false,              // showInMentions
                };
            }});
    }
}

void rebuildWhisperHighlights(Settings &settings,
                              std::vector<HighlightCheck> &checks)
{
    if (settings.enableWhisperHighlight)
    {
        auto highlightSound = settings.enableWhisperHighlightSound.getValue();
        auto highlightAlert = settings.enableWhisperHighlightTaskbar.getValue();
        auto highlightSoundUrlValue =
            settings.whisperHighlightSoundUrl.getValue();
        boost::optional<QUrl> highlightSoundUrl;
        if (!highlightSoundUrlValue.isEmpty())
        {
            highlightSoundUrl = highlightSoundUrlValue;
        }

        // The custom whisper highlight color is handled in ColorProvider

        checks.emplace_back(HighlightCheck{
            [=](const auto &args, const auto &badges, const auto &senderName,
                const auto &originalMessage,
                const auto self) -> boost::optional<HighlightResult> {
                (void)badges;           // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                (void)self;             // unused

                if (!args.isReceivedWhisper)
                {
                    return boost::none;
                }

                return HighlightResult{
                    highlightAlert,
                    highlightSound,
                    highlightSoundUrl,
                    ColorProvider::instance().color(ColorType::Whisper),
                    false,
                };
            }});
    }
}

void rebuildMessageHighlights(Settings &settings,
                              std::vector<HighlightCheck> &checks)
{
    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    QString currentUsername = currentUser->getUserName();

    if (settings.enableSelfHighlight && !currentUsername.isEmpty())
    {
        HighlightPhrase highlight(
            currentUsername, settings.showSelfHighlightInMentions,
            settings.enableSelfHighlightTaskbar,
            settings.enableSelfHighlightSound, false, false,
            settings.selfHighlightSoundUrl.getValue(),
            ColorProvider::instance().color(ColorType::SelfHighlight));

        checks.emplace_back(highlightPhraseCheck(highlight));
    }

    auto messageHighlights = settings.highlightedMessages.readOnly();
    for (const auto &highlight : *messageHighlights)
    {
        checks.emplace_back(highlightPhraseCheck(highlight));
    }
}

void rebuildUserHighlights(Settings &settings,
                           std::vector<HighlightCheck> &checks)
{
    auto userHighlights = settings.highlightedUsers.readOnly();

    for (const auto &highlight : *userHighlights)
    {
        checks.emplace_back(HighlightCheck{
            [highlight](const auto &args, const auto &badges,
                        const auto &senderName, const auto &originalMessage,
                        const auto self) -> boost::optional<HighlightResult> {
                (void)args;             // unused
                (void)badges;           // unused
                (void)originalMessage;  // unused
                (void)self;             // unused

                if (!highlight.isMatch(senderName))
                {
                    return boost::none;
                }

                boost::optional<QUrl> highlightSoundUrl;
                if (highlight.hasCustomSound())
                {
                    highlightSoundUrl = highlight.getSoundUrl();
                }

                return HighlightResult{
                    highlight.hasAlert(),       highlight.hasSound(),
                    highlightSoundUrl,          highlight.getColor(),
                    highlight.showInMentions(),
                };
            }});
    }
}

void rebuildBadgeHighlights(Settings &settings,
                            std::vector<HighlightCheck> &checks)
{
    auto badgeHighlights = settings.highlightedBadges.readOnly();

    for (const auto &highlight : *badgeHighlights)
    {
        checks.emplace_back(HighlightCheck{
            [highlight](const auto &args, const auto &badges,
                        const auto &senderName, const auto &originalMessage,
                        const auto self) -> boost::optional<HighlightResult> {
                (void)args;             // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                (void)self;             // unused

                for (const Badge &badge : badges)
                {
                    if (highlight.isMatch(badge))
                    {
                        boost::optional<QUrl> highlightSoundUrl;
                        if (highlight.hasCustomSound())
                        {
                            highlightSoundUrl = highlight.getSoundUrl();
                        }

                        return HighlightResult{
                            highlight.hasAlert(),
                            highlight.hasSound(),
                            highlightSoundUrl,
                            highlight.getColor(),
                            false,  // showInMentions
                        };
                    }
                }

                return boost::none;
            }});
    }
}

}  // namespace

namespace chatterino {

void HighlightController::initialize(Settings &settings, Paths & /*paths*/)
{
    this->rebuildListener_.addSetting(settings.enableWhisperHighlight);
    this->rebuildListener_.addSetting(settings.enableWhisperHighlightSound);
    this->rebuildListener_.addSetting(settings.enableWhisperHighlightTaskbar);
    this->rebuildListener_.addSetting(settings.whisperHighlightSoundUrl);
    this->rebuildListener_.addSetting(settings.whisperHighlightColor);
    this->rebuildListener_.addSetting(settings.enableSelfHighlight);
    this->rebuildListener_.addSetting(settings.enableSubHighlight);
    this->rebuildListener_.addSetting(settings.enableSubHighlightSound);
    this->rebuildListener_.addSetting(settings.enableSubHighlightTaskbar);

    this->rebuildListener_.setCB([this, &settings] {
        qCDebug(chatterinoHighlights)
            << "Rebuild checks because a setting changed";
        this->rebuildChecks(settings);
    });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedBadges.delayedItemsChanged,
        [this, &settings] {
            qCDebug(chatterinoHighlights)
                << "Rebuild checks because highlight badges changed";
            this->rebuildChecks(settings);
        });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedUsers.delayedItemsChanged, [this, &settings] {
            qCDebug(chatterinoHighlights)
                << "Rebuild checks because highlight users changed";
            this->rebuildChecks(settings);
        });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedMessages.delayedItemsChanged,
        [this, &settings] {
            qCDebug(chatterinoHighlights)
                << "Rebuild checks because highlight messages changed";
            this->rebuildChecks(settings);
        });

    getIApp()->getAccounts()->twitch.currentUserChanged.connect(
        [this, &settings] {
            qCDebug(chatterinoHighlights)
                << "Rebuild checks because user swapped accounts";
            this->rebuildChecks(settings);
        });

    this->rebuildChecks(settings);
}

void HighlightController::rebuildChecks(Settings &settings)
{
    // Access checks for modification
    auto checks = this->checks_.access();
    checks->clear();

    // CURRENT ORDER:
    // Subscription -> Whisper -> User -> Message -> Badge

    rebuildSubscriptionHighlights(settings, *checks);

    rebuildWhisperHighlights(settings, *checks);

    rebuildUserHighlights(settings, *checks);

    rebuildMessageHighlights(settings, *checks);

    rebuildBadgeHighlights(settings, *checks);
}

std::pair<bool, HighlightResult> HighlightController::check(
    const MessageParseArgs &args, const std::vector<Badge> &badges,
    const QString &senderName, const QString &originalMessage) const
{
    bool highlighted = false;
    auto result = HighlightResult::emptyResult();

    // Access for checking
    const auto checks = this->checks_.accessConst();

    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    auto self = (senderName == currentUser->getUserName());

    for (const auto &check : *checks)
    {
        if (auto checkResult =
                check.cb(args, badges, senderName, originalMessage, self);
            checkResult)
        {
            highlighted = true;

            if (checkResult->alert)
            {
                if (!result.alert)
                {
                    result.alert = checkResult->alert;
                }
            }

            if (checkResult->playSound)
            {
                if (!result.playSound)
                {
                    result.playSound = checkResult->playSound;
                }
            }

            if (checkResult->customSoundUrl)
            {
                if (!result.customSoundUrl)
                {
                    result.customSoundUrl = checkResult->customSoundUrl;
                }
            }

            if (checkResult->color)
            {
                if (!result.color)
                {
                    result.color = checkResult->color;
                }
            }

            if (checkResult->showInMentions)
            {
                if (!result.showInMentions)
                {
                    result.showInMentions = checkResult->showInMentions;
                }
            }

            if (result.full())
            {
                // The final highlight result does not have room to add any more parameters, early out
                break;
            }
        }
    }

    return {highlighted, result};
}

}  // namespace chatterino
