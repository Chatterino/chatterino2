// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

#include <deque>
#include <optional>
#include <vector>

namespace chatterino {

/**
 * @brief Tracks visited notebook pages for back/forward navigation.
 *
 * Stores a bounded back stack of pages the user left, and a forward stack of
 * pages available after navigating back. The currently selected page is not
 * stored in the history; callers pass it to @ref goBack and @ref goForward.
 */
class TabHistory
{
public:
    /// Records @a from as a back-stack entry and clears the forward stack.
    void recordVisit(QWidget *from);

    /// Pops the most recent back entry and optionally pushes @a current onto
    /// the forward stack.
    std::optional<QWidget *> goBack(QWidget *current);

    /// Pops the most recent forward entry and optionally pushes @a current onto
    /// the back stack.
    std::optional<QWidget *> goForward(QWidget *current);

    /// Removes all occurrences of @a page from the history.
    void removePage(QWidget *page);

    bool canGoBack() const;
    bool canGoForward() const;
    std::optional<QWidget *> peekBack() const;
    std::optional<QWidget *> peekForward() const;

    /// Returns back-stack entries from most recently visited to oldest.
    std::vector<QWidget *> backStackMostRecentFirst() const;

    /// Removes the top back entry without navigating.
    void discardBackTop();

    /// Removes the top forward entry without navigating.
    void discardForwardTop();

private:
    static constexpr size_t MAX_TAB_HISTORY_SIZE = 50;

    // history_[0, backCount_) is the back stack (oldest to newest).
    // history_[backCount_, end) is the forward stack (oldest to newest).
    std::deque<QWidget *> history_;
    size_t backCount_ = 0;
};

}  // namespace chatterino
