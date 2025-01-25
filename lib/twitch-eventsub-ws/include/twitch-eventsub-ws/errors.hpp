#pragma once

#include <boost/system/error_category.hpp>

#include <string>

namespace chatterino::eventsub::lib::error {

class ApplicationErrorCategory final : public boost::system::error_category
{
    const std::string innerMessage;

public:
    ApplicationErrorCategory(const char *_innerMessage)
        : innerMessage(_innerMessage)
    {
    }

    explicit ApplicationErrorCategory(std::string _innerMessage)
        : innerMessage(std::move(_innerMessage))
    {
    }

    const char *name() const noexcept override
    {
        return "Application JSON error";
    }
    std::string message(int /*ev*/) const override
    {
        return this->innerMessage;
    }
};

const ApplicationErrorCategory EXPECTED_OBJECT{"Expected object"};
const ApplicationErrorCategory MISSING_KEY{"Missing key"};

}  // namespace chatterino::eventsub::lib::error
