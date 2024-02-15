#pragma once

#include "messages/Image.hpp"

namespace chatterino {

/// @brief Rich info about a URL with tooltip and thumbnail
///
/// Represents info about a URL that gets resolved through the currently set
/// link resolver (Env::linkResolverUrl). Info is loaded upon first calling
/// #ensureLoadingStarted() which sets off the request. Loading can be blocked
/// by disabling the "linkInfoTooltip" setting. URLs will be unshortened if the
/// "unshortLinks" setting is enabled.
///
/// This class can only be used from the GUI thread.
class LinkInfo : public QObject
{
    Q_OBJECT

public:
    /// @brief Constructs a new link info for a URL
    ///
    /// This doesn't load any link info.
    /// @see #ensureLoadingStarted()
    [[nodiscard]] explicit LinkInfo(QString url);

    ~LinkInfo() override;

    LinkInfo(const LinkInfo &) = delete;
    LinkInfo(LinkInfo &&) = delete;
    LinkInfo &operator=(const LinkInfo &) = delete;
    LinkInfo &operator=(LinkInfo &&) = delete;

    /// @brief The URL of this link
    ///
    /// If the "unshortLinks" setting is enabled, this can change after the
    /// link is resolved.
    [[nodiscard]] QString url() const;

    /// Returns true if this link has been resolved
    [[nodiscard]] bool isResolved() const;

    /// Returns true if the info is loading
    [[nodiscard]] bool isLoading() const;

    /// Returns true if the info failed to load
    [[nodiscard]] bool hasError() const;

    /// Returns true if this link has a thumbnail
    [[nodiscard]] bool hasThumbnail() const;

    /// @brief Returns the tooltip of this link
    ///
    /// The tooltip contains the URL of the link and any info added by the
    /// resolver. Resolvers must include the URL.
    [[nodiscard]] QString tooltip() const;

    /// @brief Returns the thumbnail of this link
    ///
    /// The thumbnail is provided by the resolver and might not have been
    /// loaded yet.
    ///
    /// @pre The caller must check #hasThumbnail() before calling this method
    [[nodiscard]] ImagePtr thumbnail() const;

    /// @brief Starts to resolve the link if it hasn't been resolved yet
    ///
    /// It is safe to call this more than once and during loading.
    /// The "linkInfoTooltip" setting is checked before any request is made.
    ///
    /// #lifecycleChanged() is emitted after the link updates its state.
    void ensureLoadingStarted();

signals:
    /// Emitted when this link's lifecycle changes
    void lifecycleChanged();

private:
    enum class Lifecycle {
        /// @brief The object was created, no info is resolved
        ///
        /// This is the initial state
        Created,
        /// Info is currently loading
        Loading,
        /// Info has been resolved and the properties have been updated
        Resolved,
        /// There has been an error loading the link info (e.g. timeout)
        Errored,
    };

    /// @brief Updates the lifecycle and emits #lifecycleChanged accordingly
    ///
    /// @pre The caller must be in the GUI thread.
    void setLifecycle(Lifecycle lifecycle);

    QString url_;

    QString tooltip_;
    ImagePtr thumbnail_;

    Lifecycle lifecycle_ = Lifecycle::Created;
};

}  // namespace chatterino
