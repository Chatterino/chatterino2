#pragma once

#include "providers/links/LinkResolver.hpp"

#include <gmock/gmock.h>
#include <QString>
#include <QStringList>

namespace chatterino::mock {

class LinkResolver : public ILinkResolver
{
public:
    LinkResolver() = default;
    ~LinkResolver() override = default;

    MOCK_METHOD(void, resolve, (LinkInfo * info), (override));
};

class EmptyLinkResolver : public ILinkResolver
{
public:
    EmptyLinkResolver() = default;
    ~EmptyLinkResolver() override = default;

    void resolve(LinkInfo *info) override
    {
        //
    }
};

}  // namespace chatterino::mock
