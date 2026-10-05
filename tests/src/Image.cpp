// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/Image.hpp"

#include "controllers/emotes/EmoteController.hpp"
#include "mocks/BaseApplication.hpp"
#include "singletons/helper/GifTimer.hpp"
#include "singletons/WindowManager.hpp"
#include "Test.hpp"

#include <QApplication>
#include <QBuffer>
#include <QColor>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImage>
#include <QImageReader>
#include <QTemporaryFile>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <chrono>

using namespace chatterino;
using namespace std::chrono_literals;

namespace {

constexpr auto GIF_1X1_HEADER_HEX = "47494638396101000100800000ff00000000ff";
constexpr auto GIF_1024X1024_HEADER_HEX =
    "47494638396100040004800000ff00000000ff";
constexpr auto GIF_INFINITE_LOOP_HEX = "21ff0b4e45545343415045322e300301000000";
constexpr auto GIF_RED_20MS_FRAME_HEX =
    "21f90400020000002c0000000001000100000202440100";
constexpr auto GIF_BLUE_60MS_FRAME_HEX =
    "21f90400060000002c00000000010001000002024c0100";
constexpr char GIF_TRAILER = '\x3b';

constexpr auto TRANSPARENT_GIF_HEX =
    "47494638396101000100800000ff00000000ff"
    "21f90401020000002c00000000010001000002024401003b";

// Red for 5 ms, blue for 5 ms, then green for 1000 ms.
constexpr auto SHORT_FRAME_WEBP_HEX =
    "52494646b400000057454250565038580a00000002000000000000000000"
    "414e494d06000000000000000000414e4d46280000000000000000000000"
    "00000000050000025650384c0f0000002f000000000710fd8ffe0722a2ff0100"
    "414e4d4628000000000000000000000000000000050000005650384c0f000000"
    "2f000000000710d1fffe0722a2ff0100"
    "414e4d4628000000000000000000000000000000e80300025650384c0f000000"
    "2f0000000007d0ff88fe0722a2ff0100";

// Two 1x1 WebP frames with no delays, red then blue
constexpr auto ZERO_DELAY_WEBP_HEX =
    "524946468400000057454250565038580a00000002000000000000000000"
    "414e494d06000000000000000000414e4d46280000000000000000000000"
    "00000000000000025650384c0f0000002f000000000710fd8ffe0722a2ff0100"
    "414e4d4628000000000000000000000000000000000000005650384c0f000000"
    "2f000000000710d1fffe0722a2ff0100";

class MockApplication : public mock::BaseApplication
{
public:
    EmoteController *getEmotes() override
    {
        return &this->emotes;
    }

    EmoteController emotes;
};

class ImageApplication : public MockApplication
{
public:
    WindowManager *getWindows() override
    {
        return &this->windows;
    }

    WindowManager windows{this->getArgs(), this->getPaths(), this->settings,
                          this->theme, this->fonts};
};

QByteArray makeGifAnimation(bool loop)
{
    // Two one-pixel GIF frames: red for 20 ms, then blue for 60 ms.
    auto data = QByteArray::fromHex(GIF_1X1_HEADER_HEX);
    if (loop)
    {
        data += QByteArray::fromHex(GIF_INFINITE_LOOP_HEX);
    }
    data += QByteArray::fromHex(GIF_RED_20MS_FRAME_HEX);
    data += QByteArray::fromHex(GIF_BLUE_60MS_FRAME_HEX);
    data += GIF_TRAILER;
    return data;
}

bool hasColor(const detail::Frames &frames, QColor color)
{
    auto pixmap = frames.current();
    return pixmap && pixmap->toImage().pixelColor(0, 0) == color;
}

// Advance until the expected frame appears or the timeout expires.
bool advanceTo(detail::Frames &frames, GIFTimer &timer, QColor color)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < 5000)
    {
        frames.onPaint(std::chrono::steady_clock::now());
        timer.signal.invoke();
        if (hasColor(frames, color))
        {
            return true;
        }
        QThread::msleep(GIF_FRAME_LENGTH);
    }
    return false;
}

QPixmap makePixmap(QSize size, Qt::GlobalColor color)
{
    QPixmap pixmap(size);
    pixmap.fill(color);
    return pixmap;
}

}  // namespace

TEST(Image, DynamicFramesCache)
{
    // Images using dynamic frames share a cache separate from other images.
    MockApplication app;
    const Url url{QStringLiteral("https://example.invalid/image.webp")};
    auto image = Image::fromUrlWithDynamicFrames(url);
    EXPECT_EQ(image, Image::fromUrlWithDynamicFrames(url));
    EXPECT_NE(image, Image::fromUrl(url));
}

TEST(Image, DynamicFramesLoopAndKeepSize)
{
    // A GIF with the loop extension returns to the first frame.
    MockApplication app;
    detail::Frames frames(makeGifAnimation(true));
    ASSERT_FALSE(frames.empty());
    ASSERT_TRUE(frames.animated());
    ASSERT_TRUE(hasColor(frames, Qt::red));
    auto &timer = *app.emotes.getGIFTimer();
    ASSERT_TRUE(advanceTo(frames, timer, Qt::blue));
    // The size is still available after advancing past the first frame.
    EXPECT_EQ(frames.frameSize(), QSize(1, 1));
    EXPECT_TRUE(advanceTo(frames, timer, Qt::red));
}

TEST(Image, DynamicFramesRespectFiniteLoopCount)
{
    // A GIF with no loop extension stops on its last frame.
    MockApplication app;
    detail::Frames frames(makeGifAnimation(false));
    auto &timer = *app.emotes.getGIFTimer();
    ASSERT_TRUE(advanceTo(frames, timer, Qt::blue));
    // Wait past the last frame's 60 ms duration.
    QThread::msleep(80);
    frames.onPaint(std::chrono::steady_clock::now());
    timer.signal.invoke();
    frames.onPaint(std::chrono::steady_clock::now());
    timer.signal.invoke();
    EXPECT_TRUE(hasColor(frames, Qt::blue));
}

TEST(Image, DynamicFramesNeedSharedTimerTicks)
{
    // Dynamic frames do not advance without ticks from Chatterino's GIF timer.
    MockApplication app;
    detail::Frames frames(makeGifAnimation(true));
    frames.onPaint(std::chrono::steady_clock::now());
    QEventLoop loop;
    QTimer::singleShot(100ms, &loop, &QEventLoop::quit);
    loop.exec();
    EXPECT_TRUE(hasColor(frames, Qt::red));
}

TEST(Image, DynamicFramesPauseOffScreenAndResume)
{
    // Dynamic frames pause when they have not been painted for over 1 second.
    MockApplication app;
    detail::Frames frames(makeGifAnimation(true));
    auto &timer = *app.emotes.getGIFTimer();
    ASSERT_TRUE(advanceTo(frames, timer, Qt::blue));
    QThread::msleep(1100);
    timer.signal.invoke();
    EXPECT_TRUE(hasColor(frames, Qt::blue));
    // Painting the image again allows playback to resume.
    EXPECT_TRUE(advanceTo(frames, timer, Qt::red));
}

TEST(Image, DynamicFramesClearAndDestructionDisconnect)
{
    // Dynamic frames can be cleared and destroyed safely, and timer ticks after
    // destruction are safe.
    MockApplication app;
    auto &timer = *app.emotes.getGIFTimer();
    {
        detail::Frames frames(makeGifAnimation(true));
        frames.onPaint(std::chrono::steady_clock::now());
        frames.clear();
        EXPECT_TRUE(frames.empty());
        EXPECT_FALSE(frames.animated());
        EXPECT_FALSE(frames.frameSize());
        EXPECT_FALSE(frames.current());
        timer.signal.invoke();
        frames.clear();
    }
    {
        detail::Frames frames(makeGifAnimation(true));
        frames.onPaint(std::chrono::steady_clock::now());
    }
    timer.signal.invoke();
}

TEST(Image, InvalidDynamicFrames)
{
    // Invalid image data leaves the frames empty.
    MockApplication app;
    detail::Frames frames(QByteArrayLiteral("not an image"));
    EXPECT_TRUE(frames.empty());
    EXPECT_FALSE(frames.animated());
    EXPECT_FALSE(frames.current());
}

TEST(Image, DynamicFramesPreserveTransparency)
{
    // Transparency survives decoding.
    MockApplication app;
    detail::Frames frames(QByteArray::fromHex(TRANSPARENT_GIF_HEX));
    ASSERT_FALSE(frames.empty());
    EXPECT_FALSE(frames.animated());
    const auto pixmap = frames.current().value_or(QPixmap{});
    ASSERT_FALSE(pixmap.isNull());
    EXPECT_EQ(pixmap.toImage().pixelColor(0, 0).alpha(), 0);
}

TEST(Image, DynamicFramesExpirationAndReload)
{
    // Dynamically decoded images can be loaded and played again after their frames expire.
    ImageApplication app;
    QTemporaryFile file;
    ASSERT_TRUE(file.open());
    const auto data = makeGifAnimation(false);
    ASSERT_EQ(file.write(data), data.size());
    file.close();

    auto image = Image::fromUrlWithDynamicFrames(
        Url{QUrl::fromLocalFile(file.fileName()).toString()});
    const auto load = [&] {
        image->load();
        QElapsedTimer timeout;
        timeout.start();
        while (!image->loaded() && !image->isEmpty() &&
               timeout.elapsed() < 5000)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            QThread::msleep(1);
        }
    };
    load();
    ASSERT_TRUE(image->loaded());
    ASSERT_TRUE(image->animated());
    ImageExpirationPool::instance().freeAll();
    ASSERT_FALSE(image->loaded());
    load();
    ASSERT_TRUE(image->loaded());
    ASSERT_TRUE(image->animated());

    // A non-looping animation still stops on its last frame after reloading.
    ASSERT_TRUE(image->pixmapOrLoad());
    QThread::msleep(25);
    app.emotes.getGIFTimer()->signal.invoke();
    QThread::msleep(80);
    app.emotes.getGIFTimer()->signal.invoke();
    const auto pixmap = image->pixmapOrLoad().value_or(QPixmap{});
    ASSERT_FALSE(pixmap.isNull());
    EXPECT_EQ(pixmap.toImage().pixelColor(0, 0), QColor(Qt::blue));
    QCoreApplication::processEvents();
}

TEST(Image, DynamicFramesUseAnimationSettings)
{
    ImageApplication app;
    ASSERT_EQ(QApplication::activeWindow(), nullptr);
    const bool originalAnimate = app.settings.animateEmotes;
    const bool originalFocus = app.settings.animationsWhenFocused;
    app.settings.animateEmotes = true;
    app.settings.animationsWhenFocused = true;
    detail::Frames frames(makeGifAnimation(false));
    auto *timer = app.emotes.getGIFTimer();
    timer->initialize();
    frames.onPaint(std::chrono::steady_clock::now());
    const auto wait = [] {
        QEventLoop loop;
        QTimer::singleShot(100ms, &loop, &QEventLoop::quit);
        loop.exec();
    };
    wait();
    // animationsWhenFocused prevents playback when there is no focused window
    // or overlay.
    EXPECT_TRUE(hasColor(frames, Qt::red));
    timer->registerOpenOverlayWindow();
    app.settings.animateEmotes = false;
    wait();
    // An overlay does not allow playback when animateEmotes is disabled.
    EXPECT_TRUE(hasColor(frames, Qt::red));
    app.settings.animateEmotes = true;
    wait();
    // An overlay allows playback when animateEmotes is enabled.
    EXPECT_TRUE(hasColor(frames, Qt::blue));
    timer->unregisterOpenOverlayWindow();
    app.settings.animateEmotes = originalAnimate;
    app.settings.animationsWhenFocused = originalFocus;
}

TEST(Image, DynamicFramesAdvanceThroughShortFrames)
{
    // Playback progresses through frames shorter than the timer tick.
    if (!QImageReader::supportedImageFormats().contains(
            QByteArrayLiteral("webp")))
    {
        GTEST_SKIP() << "Qt WebP plugin is unavailable";
    }
    MockApplication app;
    detail::Frames frames(QByteArray::fromHex(SHORT_FRAME_WEBP_HEX));
    ASSERT_FALSE(frames.empty());
    ASSERT_TRUE(frames.animated());
    ASSERT_TRUE(hasColor(frames, Qt::red));
    frames.onPaint(std::chrono::steady_clock::now());
    QThread::msleep(25);
    frames.onPaint(std::chrono::steady_clock::now());
    app.emotes.getGIFTimer()->signal.invoke();
    // Decoding may exhaust the blue frame's delay and yield until the next tick.
    EXPECT_TRUE(hasColor(frames, Qt::blue) || hasColor(frames, Qt::green));
    EXPECT_TRUE(advanceTo(frames, *app.emotes.getGIFTimer(), Qt::green));
}

TEST(Image, DynamicFramesLoadBeyondCombinedFrameMemoryLimit)
{
    // Dynamic decoding can play GIFs whose combined decoded frames exceed the memory limit,
    // provided that each decoded frame and the file itself are within their limits.
    ImageApplication app;
    // Six 1024x1024 canvases occupy 24 MiB when every frame is cached.
    auto data = QByteArray::fromHex(GIF_1024X1024_HEADER_HEX);
    const auto frame = QByteArray::fromHex(GIF_RED_20MS_FRAME_HEX);
    for (int i = 0; i < 6; ++i)
    {
        data += frame;
    }
    data += GIF_TRAILER;

    QBuffer buffer(&data);
    QImageReader reader(&buffer, "gif");
    ASSERT_EQ(reader.size(), QSize(1024, 1024));
    ASSERT_EQ(reader.imageCount(), 6);
    ASSERT_LT(data.size(), Image::maxBytesRam);
    ASSERT_LT(1024 * 1024 * 4, Image::maxBytesRam);
    ASSERT_GT(1024 * 1024 * 4 * 6, Image::maxBytesRam);

    QTemporaryFile file;
    ASSERT_TRUE(file.open());
    ASSERT_EQ(file.write(data), data.size());
    file.close();
    const Url url{QUrl::fromLocalFile(file.fileName()).toString()};
    auto cachedImage = Image::fromUrl(url);
    auto dynamicImage = Image::fromUrlWithDynamicFrames(url);
    const auto load = [](const ImagePtr &image) {
        image->load();
        QElapsedTimer timeout;
        timeout.start();
        while (!image->loaded() && !image->isEmpty() &&
               timeout.elapsed() < 5000)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            QThread::msleep(1);
        }
    };
    load(cachedImage);
    // Decoding every frame at once exceeds the memory limit.
    EXPECT_TRUE(cachedImage->isEmpty());
    EXPECT_FALSE(cachedImage->loaded());
    load(dynamicImage);
    // Decoding one frame at a time stays within the limit.
    ASSERT_TRUE(dynamicImage->loaded());
    EXPECT_TRUE(dynamicImage->animated());
    EXPECT_EQ(dynamicImage->size(), QSizeF(1024, 1024));
    QCoreApplication::processEvents();
}

TEST(Image, ZeroDelayWebpAdvancesOnNextTick)
{
    // Frames with no delay advance to the next frame when the timer ticks.
    if (!QImageReader::supportedImageFormats().contains(
            QByteArrayLiteral("webp")))
    {
        GTEST_SKIP() << "Qt WebP plugin is unavailable";
    }
    MockApplication app;
    detail::Frames frames(QByteArray::fromHex(ZERO_DELAY_WEBP_HEX));
    ASSERT_TRUE(hasColor(frames, Qt::red));
    ASSERT_TRUE(frames.animated());
    frames.onPaint(std::chrono::steady_clock::now());
    app.emotes.getGIFTimer()->signal.invoke();
    EXPECT_TRUE(hasColor(frames, Qt::blue));
}

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
