#pragma once

namespace chatterino {

class LinkInfo;

class ILinkResolver
{
public:
    ILinkResolver() = default;
    virtual ~ILinkResolver() = default;
    ILinkResolver(const ILinkResolver &) = delete;
    ILinkResolver(ILinkResolver &&) = delete;
    ILinkResolver &operator=(const ILinkResolver &) = delete;
    ILinkResolver &operator=(ILinkResolver &&) = delete;

    virtual void resolve(LinkInfo *info) = 0;
};

class LinkResolver : public ILinkResolver
{
public:
    LinkResolver() = default;

    /// @brief Loads and updates the link info
    ///
    /// Calling this with an already resolved or currently loading info is a
    /// no-op. Loading can be blocked by disabling the "linkInfoTooltip"
    /// setting. URLs will be unshortened if the "unshortLinks" setting is
    /// enabled. The resolver is set through Env::linkResolverUrl.
    ///
    /// @pre @a info must not be nullptr
    void resolve(LinkInfo *info) override;
};

}  // namespace chatterino
