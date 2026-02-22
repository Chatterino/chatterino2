#include "controllers/plugins/api/Images.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "messages/Image.hpp"
#    include "messages/ImageSet.hpp"
#    include "util/Variant.hpp"

#    include <variant>

namespace {

using namespace chatterino;

template <typename T, typename... Args>
T noPermissionFn(Args... /* unused */)
{
    throw std::runtime_error("Missing network permission to create images");
}

void validateUrl(const QString &urlString)
{
    QUrl url(urlString);
    auto scheme = url.scheme();
    if (!url.isValid() || (scheme != u"http" && scheme != u"https"))
    {
        throw std::runtime_error("Invalid URL");
    }
}

ImagePtr imageCtor(const QString &urlString, sol::variadic_args args)
{
    double scale = 1;
    QSize expectedSize = {};
    if (args.size() >= 1)
    {
        scale = args.get<double>(0);
    }
    if (args.size() >= 2)
    {
        expectedSize = args.get<QSize>(1);
    }
    validateUrl(urlString);
    return Image::fromUrl(Url{urlString}, scale, expectedSize);
}

ImageSet imageSetCtor(sol::variadic_args args)
{
    auto getAt = [&](std::ptrdiff_t offset) {
        auto raw = args.get<std::variant<ImagePtr, QString>>(offset);
        return std::visit(
            variant::Overloaded{
                [](ImagePtr ptr) {
                    if (!ptr)
                    {
                        return Image::getEmpty();
                    }
                    return ptr;
                },
                [](QString urlString) {
                    validateUrl(urlString);
                    return Image::fromUrl(Url{std::move(urlString)});
                },
            },
            raw);
    };

    ImageSet set;
    if (args.size() >= 1)
    {
        set.setImage1(getAt(0));
    }
    if (args.size() >= 2)
    {
        set.setImage2(getAt(1));
    }
    if (args.size() >= 3)
    {
        set.setImage3(getAt(2));
    }
    return set;
}

}  // namespace

namespace chatterino::lua::api::images {

void createUserTypes(sol::table &c2, const Plugin &plugin)
{
    auto *imageCtorPtr =
        &noPermissionFn<ImagePtr, const QString &, sol::variadic_args>;
    auto *imageSetCtorPtr = &noPermissionFn<ImageSet, sol::variadic_args>;
    if (plugin.hasNetworkPermission())
    {
        imageCtorPtr = &imageCtor;
        imageSetCtorPtr = &imageSetCtor;
    }

    c2.new_usertype<Image>(
        "Image", sol::no_constructor,  //
        "from_url", imageCtorPtr,      //
        "empty",
        [] {
            return Image::getEmpty();
        },
        "url", sol::property([](const Image &img) {
            return img.url().string;
        }),                                          //
        "is_loaded", sol::property(&Image::loaded),  //
        "is_empty", sol::property(&Image::isEmpty),  //
        "width", sol::property(&Image::width),       //
        "height", sol::property(&Image::height),     //
        "scale", sol::property(&Image::scale),       //
        "size", sol::property(&Image::size),         //
        "animated", sol::property(&Image::animated)  //
    );

    c2.new_usertype<ImageSet>(
        "ImageSet", sol::factories(imageSetCtorPtr),                          //
        "image1", sol::property(&ImageSet::getImage1, &ImageSet::setImage1),  //
        "image2", sol::property(&ImageSet::getImage2, &ImageSet::setImage2),  //
        "image3", sol::property(&ImageSet::getImage3, &ImageSet::setImage3)   //
    );
}

}  // namespace chatterino::lua::api::images

#endif
