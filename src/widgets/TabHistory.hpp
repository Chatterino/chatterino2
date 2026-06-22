// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

#include <optional>
#include <vector>

namespace chatterino {

class TabHistory
{
public:
    void recordVisit(QWidget *from, QWidget *to);
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
    static void removeFromStack(std::vector<QWidget *> &stack, QWidget *page);

    std::vector<QWidget *> back_;
    std::vector<QWidget *> forward_;
};

}  // namespace chatterino
