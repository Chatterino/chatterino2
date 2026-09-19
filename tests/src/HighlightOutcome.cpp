// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/Sounds.hpp"
#include "controllers/highlights/types/All.hpp"
#include "controllers/highlights/types/Common.hpp"
#include "Test.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

using namespace chatterino;

namespace chatterino::highlights {

TEST(HighlightOutcome, MessageDefault)
{
    rapidjson::Document d;
    d.Parse(R"({"id":"test-01", "type":"message"})");

    bool error = false;
    auto h = pajlada::Deserialize<AllHighlights>::get(d, &error);

    ASSERT_FALSE(error);

    ASSERT_TRUE(std::holds_alternative<MessageHighlight>(h));

    // No sound is configured
    ASSERT_TRUE(getSoundWithoutDefault(h).isEmpty());
    ASSERT_TRUE(getSound(h).isEmpty());
    ASSERT_TRUE(getSoundURL(h).isEmpty());

    ASSERT_FALSE(shouldPlaySound(h));
}

TEST(HighlightOutcome, MessageCustomSound)
{
    rapidjson::Document d;
    d.Parse(
        R"({"id":"test-01", "type":"message", "sound":"file:///home/pajlada/Audio/karl kons oh oh no no.mp3"})");

    bool error = false;
    auto h = pajlada::Deserialize<AllHighlights>::get(d, &error);

    ASSERT_FALSE(error);

    ASSERT_TRUE(std::holds_alternative<MessageHighlight>(h));

    // User has changed to a custom sound
    ASSERT_EQ(getSound(h),
              u"file:///home/pajlada/Audio/karl kons oh oh no no.mp3");
    ASSERT_EQ(getSoundWithoutDefault(h),
              u"file:///home/pajlada/Audio/karl kons oh oh no no.mp3");
    ASSERT_EQ(getSoundURL(h),
              QUrl{"file:///home/pajlada/Audio/karl kons oh oh no no.mp3"});

    ASSERT_TRUE(shouldPlaySound(h));
}

TEST(HighlightOutcome, SubscribedThreadDefault)
{
    rapidjson::Document d;
    d.Parse(R"({"id":"subscribedthread"})");

    bool error = false;
    auto h = pajlada::Deserialize<AllHighlights>::get(d, &error);

    ASSERT_FALSE(error);

    ASSERT_TRUE(std::holds_alternative<SubscribedThreadHighlight>(h));

    // No sound is configured, but it has a default sound
    ASSERT_EQ(getSoundWithoutDefault(h), QString());
    ASSERT_EQ(getSound(h), SubscribedThreadHighlight::SOUND_DEFAULT);
    ASSERT_FALSE(getSoundURL(h).isEmpty());
    ASSERT_EQ(getSoundURL(h).toString(),
              defaultSounds().at("001-ping2").resourcePath);

    ASSERT_TRUE(shouldPlaySound(h));
}

TEST(HighlightOutcome, SubscribedThreadDisabledSound)
{
    rapidjson::Document d;
    d.Parse(R"({"id":"subscribedthread", "sound":""})");

    bool error = false;
    auto h = pajlada::Deserialize<AllHighlights>::get(d, &error);

    ASSERT_FALSE(error);

    ASSERT_TRUE(std::holds_alternative<SubscribedThreadHighlight>(h));

    // User has explicitly disabled the sound
    ASSERT_EQ(getSound(h), QString(""));
    ASSERT_TRUE(getSoundURL(h).isEmpty());

    ASSERT_FALSE(shouldPlaySound(h));
}

TEST(HighlightOutcome, SubscribedThreadOtherBuiltInSound)
{
    rapidjson::Document d;
    d.Parse(
        R"({"id":"subscribedthread", "sound":"002-sadiquecat-c4-harmonic"})");

    bool error = false;
    auto h = pajlada::Deserialize<AllHighlights>::get(d, &error);

    ASSERT_FALSE(error);

    ASSERT_TRUE(std::holds_alternative<SubscribedThreadHighlight>(h));

    // User has changed to a different built-in sound
    ASSERT_EQ(getSound(h), u"002-sadiquecat-c4-harmonic");
    ASSERT_EQ(getSoundWithoutDefault(h), u"002-sadiquecat-c4-harmonic");
    ASSERT_EQ(getSoundURL(h), QUrl{"qrc:/sounds/sadiquecat-c4-harmonic.wav"});

    ASSERT_TRUE(shouldPlaySound(h));
}

TEST(HighlightOutcome, SubscribedThreadCustomSound)
{
    rapidjson::Document d;
    d.Parse(
        R"({"id":"subscribedthread", "sound":"file:///home/pajlada/Audio/karl kons oh oh no no.mp3"})");

    bool error = false;
    auto h = pajlada::Deserialize<AllHighlights>::get(d, &error);

    ASSERT_FALSE(error);

    ASSERT_TRUE(std::holds_alternative<SubscribedThreadHighlight>(h));

    // User has changed to a custom sound
    ASSERT_EQ(getSound(h),
              u"file:///home/pajlada/Audio/karl kons oh oh no no.mp3");
    ASSERT_EQ(getSoundWithoutDefault(h),
              u"file:///home/pajlada/Audio/karl kons oh oh no no.mp3");
    ASSERT_EQ(getSoundURL(h),
              QUrl{"file:///home/pajlada/Audio/karl kons oh oh no no.mp3"});

    ASSERT_TRUE(shouldPlaySound(h));
}

}  // namespace chatterino::highlights
