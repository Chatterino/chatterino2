#include "providers/links/LinkInfo.hpp"

#include "common/Literals.hpp"
#include "SignalSpy.hpp"
#include "Test.hpp"

using namespace chatterino;
using namespace literals;

using State = LinkInfo::State;

TEST(LinkInfo, initialState)
{
    LinkInfo info(u"https://chatterino.com"_s);
    ASSERT_EQ(info.url(), u"https://chatterino.com"_s);
    ASSERT_EQ(info.originalUrl(), u"https://chatterino.com"_s);
    ASSERT_EQ(info.state(), State::Created);
    ASSERT_FALSE(info.hasThumbnail());
    ASSERT_EQ(info.tooltip(), u"https://chatterino.com"_s);
}

TEST(LinkInfo, states)
{
    LinkInfo info(u"https://chatterino.com"_s);
    SignalSpy<State> spy(&info, &LinkInfo::stateChanged);

    info.setState(State::Created);
    ASSERT_TRUE(spy.empty());
    ASSERT_TRUE(info.isPending());
    ASSERT_FALSE(info.isLoading());
    ASSERT_FALSE(info.isLoaded());
    ASSERT_FALSE(info.isResolved());
    ASSERT_FALSE(info.hasError());

    info.setState(State::Loading);
    ASSERT_EQ(spy.size(), 1);
    ASSERT_EQ(spy.last(), State::Loading);
    ASSERT_EQ(info.state(), State::Loading);
    ASSERT_FALSE(info.isPending());
    ASSERT_TRUE(info.isLoading());
    ASSERT_FALSE(info.isLoaded());
    ASSERT_FALSE(info.isResolved());
    ASSERT_FALSE(info.hasError());

    info.setState(State::Loading);
    ASSERT_EQ(spy.size(), 1);

    info.setState(State::Resolved);
    ASSERT_EQ(spy.size(), 2);
    ASSERT_EQ(spy.last(), State::Resolved);
    ASSERT_EQ(info.state(), State::Resolved);
    ASSERT_FALSE(info.isPending());
    ASSERT_FALSE(info.isLoading());
    ASSERT_TRUE(info.isLoaded());
    ASSERT_TRUE(info.isResolved());
    ASSERT_FALSE(info.hasError());

    info.setState(State::Errored);
    ASSERT_EQ(spy.size(), 3);
    ASSERT_EQ(spy.last(), State::Errored);
    ASSERT_EQ(info.state(), State::Errored);
    ASSERT_FALSE(info.isPending());
    ASSERT_FALSE(info.isLoading());
    ASSERT_TRUE(info.isLoaded());
    ASSERT_FALSE(info.isResolved());
    ASSERT_TRUE(info.hasError());
}

TEST(LinkInfo, setters)
{
    LinkInfo info(u"https://chatterino.com"_s);
    SignalSpy<State> spy(&info, &LinkInfo::stateChanged);

    ASSERT_EQ(info.url(), u"https://chatterino.com"_s);
    ASSERT_EQ(info.originalUrl(), u"https://chatterino.com"_s);
    ASSERT_FALSE(info.hasThumbnail());
    ASSERT_EQ(info.tooltip(), u"https://chatterino.com"_s);

    info.setTooltip(u"tooltip"_s);
    ASSERT_EQ(info.tooltip(), u"tooltip"_s);

    info.setResolvedUrl(u"https://www.chatterino.com"_s);
    ASSERT_EQ(info.url(), u"https://www.chatterino.com"_s);
    ASSERT_EQ(info.originalUrl(), u"https://chatterino.com"_s);

    info.setThumbnail(nullptr);
    ASSERT_FALSE(info.hasThumbnail());

    info.setThumbnail(Image::getEmpty());
    ASSERT_FALSE(info.hasThumbnail());

    info.setThumbnail(Image::fromUrl(Url{""}));
    ASSERT_FALSE(info.hasThumbnail());

    auto image = Image::fromUrl(Url{"https://www.chatterino.com"});
    info.setThumbnail(image);
    ASSERT_TRUE(info.hasThumbnail());
    ASSERT_EQ(info.thumbnail(), image);

    ASSERT_TRUE(spy.empty());
}
