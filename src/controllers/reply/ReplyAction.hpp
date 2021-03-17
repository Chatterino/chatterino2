#pragma once

#include <QString>
#include <boost/optional.hpp>
#include <pajlada/serialize.hpp>

#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

class Image;
using ImagePtr = std::shared_ptr<Image>;

class ReplyAction
{
public:
    ReplyAction();

    const boost::optional<ImagePtr> &getImage() const;

private:
    mutable boost::optional<ImagePtr> image_;
    int imageToLoad_{};
};

} // namespace chatterino
