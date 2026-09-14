// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/Image.hpp"

#include "controllers/emotes/EmoteController.hpp"
#include "mocks/BaseApplication.hpp"
#include "singletons/helper/GifTimer.hpp"
#include "Test.hpp"

#include <QColor>

using namespace chatterino;

namespace {

class ImageApplication : public mock::BaseApplication
{
public:
    EmoteController *getEmotes() override
    {
        return &this->emotes;
    }

    EmoteController emotes;
};

QPixmap makePixmap(QSize size, Qt::GlobalColor color)
{
    QPixmap pixmap(size);
    pixmap.fill(color);
    return pixmap;
}

bool hasColor(const detail::Frames &frames, QColor color)
{
    const auto pixmap = frames.current();
    return pixmap && pixmap->toImage().pixelColor(0, 0) == color;
}

}  // namespace

TEST(Image, EmptyCachedFrames)
{
    ImageApplication app;
    detail::Frames frames;
    EXPECT_TRUE(frames.empty());
    EXPECT_FALSE(frames.animated());
    EXPECT_FALSE(frames.current());
    EXPECT_FALSE(frames.frameSize());
}

TEST(Image, StaticCachedFrame)
{
    ImageApplication app;
    detail::Frames frames(QList<detail::Frame>{
        {.image = makePixmap({1, 1}, Qt::red), .duration = 20},
    });
    EXPECT_FALSE(frames.empty());
    EXPECT_FALSE(frames.animated());

    app.emotes.getGIFTimer()->signal.invoke();
    EXPECT_TRUE(hasColor(frames, Qt::red));
}

TEST(Image, CachedFramesUseSharedTimer)
{
    ImageApplication app;
    detail::Frames frames(QList<detail::Frame>{
        {.image = makePixmap({1, 1}, Qt::red), .duration = 20},
        {.image = makePixmap({2, 2}, Qt::blue), .duration = 60},
    });
    ASSERT_TRUE(frames.animated());
    ASSERT_EQ(frames.frameSize(), QSize(1, 1));

    auto *timer = app.emotes.getGIFTimer();
    timer->signal.invoke();
    EXPECT_TRUE(hasColor(frames, Qt::red));
    timer->signal.invoke();
    EXPECT_TRUE(hasColor(frames, Qt::blue));
    EXPECT_EQ(frames.frameSize(), QSize(1, 1));
}

TEST(Image, CachedFramesClearDisconnectsTimer)
{
    ImageApplication app;
    auto *timer = app.emotes.getGIFTimer();
    {
        detail::Frames frames(QList<detail::Frame>{
            {.image = makePixmap({1, 1}, Qt::red), .duration = 20},
            {.image = makePixmap({1, 1}, Qt::blue), .duration = 60},
        });
        frames.clear();
        EXPECT_TRUE(frames.empty());
        EXPECT_FALSE(frames.animated());
        EXPECT_FALSE(frames.current());
        EXPECT_FALSE(frames.frameSize());
        timer->signal.invoke();
    }
    timer->signal.invoke();
}
