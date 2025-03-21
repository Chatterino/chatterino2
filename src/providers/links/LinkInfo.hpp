#pragma once

#include "messages/Image.hpp"

namespace chatterino {

/// @brief Rich info about a URL with tooltip and thumbnail
///
/// This is only a data class - it doesn't do the resolving.
/// It can only be used from the GUI thread.
class LinkInfo : public QObject
{
    Q_OBJECT

public:
    /// @brief the state of a link info
    ///
    /// The state of a link can only increase. For example, it's not possible
    /// for the link to change from "Resolved" to "Loading".
    enum class State {
        /// @brief The object was created, no info is resolved
        ///
        /// This is the initial state
        Created,
        /// Info is currently loading
        Loading,
        /// Info has been resolved and the properties have been updated
        Resolved,
        /// There has been an error resolving the link info (e.g. timeout)
        Errored,
    };

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

    /// @brief The URL of this link as seen in the message
    ///
    /// If the "unshortLinks" setting doesn't affect this URL.
    [[nodiscard]] QString originalUrl() const;

    /// Returns the current state
    [[nodiscard]] State state() const;

    /// Returns true if this link has not yet been resolved (it's "Created")
    [[nodiscard]] bool isPending() const;

    /// Returns true if the info is loading
    [[nodiscard]] bool isLoading() const;

    /// Returns true if the info is loaded (resolved or errored)
    [[nodiscard]] bool isLoaded() const;

    /// Returns true if this link has been resolved
    [[nodiscard]] bool isResolved() const;

    /// Returns true if the info failed to resolve
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

    /// @brief Updates the state and emits #stateChanged accordingly
    ///
    /// @pre The caller must be in the GUI thread.
    /// @pre @a state must be greater or equal to the current state.
    /// @see #state(), #stateChanged
    void setState(State state);

    /// @brief Updates the resolved url of this link
    ///
    /// @pre The caller must be in the GUI thread.
    /// @see #url()
    void setResolvedUrl(QString resolvedUrl);

    /// @brief Updates the tooltip of this link
    ///
    /// @pre The caller must be in the GUI thread.
    /// @see #tooltip()
    void setTooltip(QString tooltip);

    /// @brief Updates the thumbnail of this link
    ///
    /// The thumbnail is allowed to be empty or nullptr.
    ///
    /// @pre The caller must be in the GUI thread.
    /// @see #hasThumbnail(), #thumbnail()
    void setThumbnail(ImagePtr thumbnail);

Q_SIGNALS:
    /// @brief Emitted when this link's state changes
    ///
    /// @param state The new state
    void stateChanged(State state);

private:
    const QString originalUrl_;
    QString url_;

    QString tooltip_;
    ImagePtr thumbnail_;

    State state_ = State::Created;
};

}  // namespace chatterino
