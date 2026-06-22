// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

#include <deque>
#include <optional>
#include <vector>

namespace chatterino {

class TabHistory
{
public:
    void recordVisit(QWidget *from);
    std::optional<QWidget *> goBack(QWidget *current);
    std::optional<QWidget *> goForward(QWidget *current);
    void removePage(QWidget *page);
    bool canGoBack() const;
    bool canGoForward() const;
    std::optional<QWidget *> peekBack() const;
    std::optional<QWidget *> peekForward() const;
    std::vector<QWidget *> backStackMostRecentFirst() const;
    void discardBackTop();
    void discardForwardTop();

private:
    static constexpr size_t MAX_TAB_HISTORY_SIZE = 50;

    // history_[0, backCount_) is the back stack (oldest to newest).
    // history_[backCount_, end) is the forward stack (oldest to newest).
    std::deque<QWidget *> history_;
    size_t backCount_ = 0;
};

}  // namespace chatterino
