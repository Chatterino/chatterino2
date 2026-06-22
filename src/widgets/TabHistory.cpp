// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/TabHistory.hpp"

#include <algorithm>

namespace {

constexpr size_t MAX_TAB_HISTORY_SIZE = 50;

}  // namespace

namespace chatterino {

void TabHistory::recordVisit(QWidget *from, QWidget *to)
{
    if (from == nullptr || to == nullptr || from == to)
    {
        return;
    }

    this->back_.push_back(from);
    if (this->back_.size() > MAX_TAB_HISTORY_SIZE)
    {
        this->back_.erase(this->back_.begin());
    }

    this->forward_.clear();
}

std::optional<QWidget *> TabHistory::goBack(QWidget *current)
{
    if (this->back_.empty())
    {
        return std::nullopt;
    }

    QWidget *target = this->back_.back();
    this->back_.pop_back();

    if (current != nullptr)
    {
        this->forward_.push_back(current);
    }

    return target;
}

std::optional<QWidget *> TabHistory::goForward(QWidget *current)
{
    if (this->forward_.empty())
    {
        return std::nullopt;
    }

    QWidget *target = this->forward_.back();
    this->forward_.pop_back();

    if (current != nullptr)
    {
        this->back_.push_back(current);
    }

    return target;
}

void TabHistory::removePage(QWidget *page)
{
    removeFromStack(this->back_, page);
    removeFromStack(this->forward_, page);
}

bool TabHistory::canGoBack() const
{
    return !this->back_.empty();
}

bool TabHistory::canGoForward() const
{
    return !this->forward_.empty();
}

std::optional<QWidget *> TabHistory::peekBack() const
{
    if (this->back_.empty())
    {
        return std::nullopt;
    }

    return this->back_.back();
}

std::optional<QWidget *> TabHistory::peekForward() const
{
    if (this->forward_.empty())
    {
        return std::nullopt;
    }

    return this->forward_.back();
}

std::vector<QWidget *> TabHistory::backStackMostRecentFirst() const
{
    return {this->back_.rbegin(), this->back_.rend()};
}

void TabHistory::discardBackTop()
{
    if (!this->back_.empty())
    {
        this->back_.pop_back();
    }
}

void TabHistory::discardForwardTop()
{
    if (!this->forward_.empty())
    {
        this->forward_.pop_back();
    }
}

void TabHistory::removeFromStack(std::vector<QWidget *> &stack, QWidget *page)
{
    stack.erase(std::remove(stack.begin(), stack.end(), page), stack.end());
}

}  // namespace chatterino
