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
 * Stores a linear history with a current index. Entries before the current
 * index are the back stack; entries after it are the forward stack.
 */
class TabHistory
{
public:
    /// Maximum number of page pointers stored in @ref history_.
    static constexpr size_t MAX_TAB_HISTORY_SIZE = 50;

    /// Records navigation from @a from to @a to, clearing forward entries.
    void recordNavigation(QWidget *from, QWidget *to);

    /// Moves to the previous entry and returns it.
    std::optional<QWidget *> goBack();

    /// Moves to the next entry and returns it.
    std::optional<QWidget *> goForward();

    /// Removes all occurrences of @a page from the history.
    void removePage(QWidget *page);

    bool canGoBack() const;
    bool canGoForward() const;
    std::optional<QWidget *> peekBack() const;
    std::optional<QWidget *> peekForward() const;

    /// Returns back-stack entries from most recently visited to oldest.
    std::vector<QWidget *> backStackMostRecentFirst() const;

    /// Removes the most recent back entry (the one @ref peekBack would return).
    void discardBackTop();

    /// Removes the next forward entry (the one @ref peekForward would return).
    void discardForwardTop();

private:
    // history_[0, currentIndex_] is the back stack plus the current page.
    // history_[currentIndex_ + 1, end) is the forward stack.
    std::deque<QWidget *> history_;
    size_t currentIndex_ = 0;
};

}  // namespace chatterino
